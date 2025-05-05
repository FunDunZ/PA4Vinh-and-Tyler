#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "softwaredisk.h"
#include "filesystem.h"

//Define a new minimum function
  #define min(a, b) ((a) < (b) ? (a) : (b))

// Define the global error variable
FSError fserror = FS_NONE;

struct FileInternals {
    int inode_index;            // index in inode table
    FileMode user_mode;
    unsigned long position;    // current position in file                  

};


// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode) {
    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return NULL;
    }

    //Now that we have confirmed the name is valid, we need to make sure the file exists
    int dir_index = find_name_dir_entry(name);
    if (dir_index == -1) {
        fserror = FS_FILE_NOT_FOUND;
        return NULL;
    }

    //Now we need to get the directory entry to eventually get the inode
    DirEntry fileEntry;
    if (!get_dir_entry(dir_index, &fileEntry)) {
        fserror = FS_IO_ERROR;
        return NULL;
    }
    //Now that we have gotten the directory entry we need the inode
    int inode_num = fileEntry.inode_number;

    //Now we need to set all of the file parameters
    File exFile = malloc(sizeof(struct FileInternals));
    exFile->inode_index = inode_num;
    exFile->user_mode = mode;
    exFile->position = 0;


    fserror = FS_NONE;
    return (File)exFile;
}

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name) {
    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return NULL;
    }

    //Now we need to check to make sure the file doesn't already exist
    if (file_exists(name)) {
        fserror = FS_FILE_ALREADY_EXISTS;
        return NULL;
    }

    //Now we need to create the directory entry to eventually get the inode
    int dir_index = find_free_dir_entry();
    if (dir_index == -1) {
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }
 

    //Now that we have created the directory entry we need to create the new inode
    int inode_num = find_free_inode();
    if (inode_num == -1) {
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }
 
    //Now we need to allocate the new inode amd create the new one
    allocate_inode(inode_num);
    Inode newInode;
    newInode.inode_number = inode_num;
    newInode.file_size = 0;
    for (int i = 0; i < NUM_DIRECT_INODE_BLOCKS; i++) {
        newInode.direct_blocks[i] = 0;
    }
    newInode.indirect_block = 0;

    //Now we need to write it to our disk
    if (!write_inode(inode_num, &newInode)) {
        fserror = FS_IO_ERROR;
        return NULL;
    }

    //Lastly before creating the file, we add the directory entry
    if (!add_dir_entry(name, inode_num)) {
        fserror = FS_OUT_OF_SPACE;
        return NULL;
    }

    //Now we need to create the new file
    File exFile = malloc(sizeof(struct FileInternals));
    if (exFile == NULL) {
        fserror = FS_IO_ERROR;
        return NULL;
    }
    exFile->inode_index = inode_num;
    exFile->user_mode = READ_WRITE;
    exFile->position = 0;

    //Now that we have created our new file, we return the pointer to the file
    fserror = FS_NONE;
    return (File)exFile;
}

// close 'file'.  Always sets 'fserror' global.
void close_file(File file) {
    //First we need to check to make sure the file is valid
    if (file == NULL) {
        fserror = FS_FILE_NOT_OPEN;
        return;
    }

    //Now that we have confirmed that the file is valid, we free it and proceed
    free(file);

    fserror = FS_NONE;
    return;
}

// read at most 'numbytes' of data from 'file' into 'buf', starting at the 
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes) {
    //First we need to check to make sure the file is valid
    if (file == NULL) {
        fserror = FS_FILE_NOT_OPEN;
        return 0;
    }

    //Now we need to get the indoe so that way we can get the information
    Inode inodeOfFile;
    if (!get_inode(file->inode_index, &inodeOfFile)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we need to make sure our posiition is valid
    if (file->position >= inodeOfFile.file_size) {
        fserror = FS_OUT_OF_SPACE;
        return 0;
    }

    //Now we need to figure out the bytes to read
    unsigned long bytes_available = inodeOfFile.file_size - file->position;
    unsigned long bytes_to_read = min(numbytes, bytes_available);
    unsigned long bytes_read = 0;
    unsigned char *dst = (unsigned char *)buf;

    //Here we set up a while loop to ensure that we read all bytes needed
    while (bytes_read < bytes_to_read) {
        unsigned long cur_position = file->position + bytes_read;
        int blk = cur_position / SOFTWARE_DISK_BLOCK_SIZE;
        int off = cur_position % SOFTWARE_DISK_BLOCK_SIZE;

        int blk_num;
        if (blk < NUM_DIRECT_INODE_BLOCKS) {
            blk_num = inodeOfFile.direct_blocks[blk];
        }
        else {
            int indirect_block_array[NUM_SINGLE_INDIRECT_BLOCKS];
            int indirect_loaded = 0;
            if (!indirect_loaded) {
                if (!read_sd_block(indirect_block_array, inodeOfFile.indirect_block)) {
                    fserror = FS_IO_ERROR;
                    return bytes_read;
                }
                indirect_loaded = 1;
            }
            int indir_indx = blk - NUM_DIRECT_INODE_BLOCKS;
            blk_num = indirect_block_array[indir_indx];
        }

        unsigned char blk_buf[SOFTWARE_DISK_BLOCK_SIZE];
        if (!read_sd_block(blk_buf, blk_num)) {
            fserror = FS_IO_ERROR;
            return bytes_read;
        }

        int bytes_in_blk = min(SOFTWARE_DISK_BLOCK_SIZE - off, bytes_to_read - bytes_read);

        memcpy(dst + bytes_read, blk_buf + off, bytes_in_blk);

        bytes_read += bytes_in_blk;
    }
    file->position += bytes_read;

    fserror = FS_NONE;
    return bytes_read;
}

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes) {
    //First we need to check to make sure the file is valid
    if (file == NULL) {
        fserror = FS_FILE_NOT_OPEN;
        return 0;
    }

    //Now we need to check to make sure the mode is correct
    if (file->user_mode != READ_WRITE) {
        fserror = FS_FILE_READ_ONLY;
        return 0;
    }

    //Now we need to get the indoe so that way we can get the information
    Inode inodeOfFile;
    if (!get_inode(file->inode_index, &inodeOfFile)) {
        fserror = FS_IO_ERROR;
        return 0;
    }   

    //Now we need to find the total bytes the file can hold
    unsigned long max_bytes = MAX_FILE_SIZE - file->position;
    unsigned long bytes_to_write = min(numbytes, max_bytes);
    if (bytes_to_write == 0) {
        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }

    unsigned long bytes_written = 0;
    unsigned char *source = (unsigned char *)buf;

    //Now we need to do the loop for the write, under a while so we finish the whole write
    while (bytes_written < bytes_to_write) {
        unsigned long cur_position = file->position + bytes_written;
        int blk = cur_position / SOFTWARE_DISK_BLOCK_SIZE;
        int off = cur_position % SOFTWARE_DISK_BLOCK_SIZE;

        //Here we are going to do all of the block information with direct and indirect
        int blk_num;
        if (blk < NUM_DIRECT_INODE_BLOCKS) {
            if (inodeOfFile.direct_blocks[blk] == 0) {
                int new_blk = find_free_data_block();
                if (new_blk == -1) {
                    fserror = FS_OUT_OF_SPACE;
                    break;
                }
                allocate_data_block(new_blk);
                inodeOfFile.direct_blocks[blk] = new_blk;
            }
            blk_num = inodeOfFile.direct_blocks[blk];
        }
        else {
            if (inodeOfFile.indirect_block == 0) {
                int new_indir = find_free_data_block();
                if (new_indir == -1) {
                    fserror = FS_OUT_OF_SPACE;
                    break;
                }
                allocate_data_block(new_indir);
                inodeOfFile.indirect_block = new_indir;

                int zeros[NUM_SINGLE_INDIRECT_BLOCKS] = {0};
                write_sd_block(zeros, new_indir);
            }
            int indir_blk_array[NUM_SINGLE_INDIRECT_BLOCKS];
            read_sd_block(indir_blk_array, inodeOfFile.indirect_block);
            int indir_indx = blk - NUM_DIRECT_INODE_BLOCKS;
            if (indir_blk_array[indir_indx] == 0) {
                int new_blk = find_free_data_block();
                if (new_blk == -1) {
                    fserror = FS_OUT_OF_SPACE;
                    break;
                }
                allocate_data_block(new_blk);
                indir_blk_array[indir_indx] = new_blk;
                write_sd_block(indir_blk_array, inodeOfFile.indirect_block);
            }
            blk_num = indir_blk_array[indir_indx];
        }

        //Now we read the block from the disk
        unsigned char blk_buf[SOFTWARE_DISK_BLOCK_SIZE];
        read_sd_block(blk_buf, blk_num);

        //How many bytes are in the block
        int bytes_in_blk = min(SOFTWARE_DISK_BLOCK_SIZE - off, bytes_to_write - bytes_written);

        //Copy from the source to write into the block buffer so it can be written
        memcpy(blk_buf + off, source + bytes_written, bytes_in_blk);

        //Now we write the block into the disk again
        write_sd_block(blk_buf, blk_num);

        //Add to the bytes written
        bytes_written += bytes_in_blk;
    }

    //Set our new position in the file to reflect how far we wrote
    unsigned long new_pos = file->position + bytes_written;
    if (new_pos > inodeOfFile.file_size) {
        inodeOfFile.file_size = new_pos;
    }

    file->position = new_pos;

    //Now we write the inode back to the file and check to make sure we didn't run out of space
    write_inode(file->inode_index, &inodeOfFile);

    if (bytes_written == bytes_to_write) {
        fserror = FS_NONE;
    }
    else  {
        fserror = FS_OUT_OF_SPACE;
    }
    return bytes_written;
}

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos) {
    //First we need to check to make sure the file is valid
    if (file == NULL) {
        fserror = FS_FILE_NOT_OPEN;
        return 0;
    }

    //Now we need to verify our byteposition is valid
    if (bytepos > MAX_FILE_SIZE) {
        fserror = FS_EXCEEDS_MAX_FILE_SIZE;
        return 0;
    }

    //Now we need to get the inode
    Inode inodeOfFile;
    if (!get_inode(file->inode_index, &inodeOfFile)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Check if we need to extend the file in any way
    if (bytepos > inodeOfFile.file_size) {
        unsigned long old_size = inodeOfFile.file_size;
        unsigned long old_last_blk;
        unsigned long new_last_blk;

        if (old_size == 0) {
            old_last_blk = -1;
        }
        else {
            old_last_blk = (old_size - 1) / SOFTWARE_DISK_BLOCK_SIZE;
        }

        if (bytepos == 0) {
            new_last_blk = 0;
        }
        else {
            new_last_blk = (bytepos - 1) / SOFTWARE_DISK_BLOCK_SIZE;
        }

        for (unsigned long blk = old_last_blk + 1; blk <= new_last_blk; blk++) {
            if (blk < NUM_DIRECT_INODE_BLOCKS) {
                if (inodeOfFile.direct_blocks[blk] == 0) {
                    int new_blk = find_free_data_block();
                    if (new_blk == -1) {
                        fserror = FS_OUT_OF_SPACE;
                        return 0;
                    }
                    allocate_data_block(new_blk);
                    inodeOfFile.direct_blocks[blk] = new_blk;
                }
            }
            else {
                if (inodeOfFile.indirect_block == 0) {
                    int new_indir = find_free_data_block();
                    if (new_indir == -1) {
                        fserror = FS_OUT_OF_SPACE;
                        return 0;
                    }
                    allocate_data_block(new_indir);
                    inodeOfFile.indirect_block = new_indir;

                    int zeros[NUM_SINGLE_INDIRECT_BLOCKS] = {0};
                    write_sd_block(zeros, new_indir);
                }
                int indir_blk_array[NUM_SINGLE_INDIRECT_BLOCKS];
                read_sd_block(indir_blk_array, inodeOfFile.indirect_block);
                int indir_indx = blk - NUM_DIRECT_INODE_BLOCKS;
                if (indir_blk_array[indir_indx] == 0) {
                    int new_blk = find_free_data_block();
                    if (new_blk == -1) {
                        fserror = FS_OUT_OF_SPACE;
                        return 0;
                    }
                    allocate_data_block(new_blk);
                    indir_blk_array[indir_indx] = new_blk;
                    write_sd_block(indir_blk_array, inodeOfFile.indirect_block);
                }
            }
        }
        inodeOfFile.file_size = bytepos;
        write_inode(file->inode_index, &inodeOfFile);
    }
    file->position = bytepos;

    fserror = FS_NONE;
    return 1;
}

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file) {
    //First we need to check to make sure the file exists
    if (file == NULL) {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }

    //Now that we know the file exists, we need to get the inode
    int fileInode = file->inode_index;
    Inode exInode;

    //Now we need to get the inode
    //Nest with an if to check for IO error

    if (!get_inode(fileInode, &exInode)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //If we have made it here, we saved our inode info from our file
    //into the exInode of type Inode

    //Now we need to read the size and return it
    return exInode.file_size;
}

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name) {
    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return 0;
    }

    //Now we need to check to make sure that the file exists
    if (!file_exists(name)) {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }

    //***** I THINK WE NEED TO CHECK IF THE FILE IS OPEN, BUT IDK HOW


    //Now we have confirmed that the file exists and the name is valid
    //Now we must proceed with the deletion

    //First we need to find the directory index
    int dir_index = find_name_dir_entry(name);
    if (dir_index == -1) {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }

    //Now we need to get the directory entry at that index in order to get the inode
    DirEntry dirForFileDelete;
    if (!get_dir_entry(dir_index, &dirForFileDelete)) {
        fserror = FS_IO_ERROR;
        return 0;
    }
    int inode_number = dirForFileDelete.inode_number;

    //Now we need to find the inode that contains our file
    Inode fileInode;
    if (!get_inode(inode_number, &fileInode)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we need to free the data blocks, both direct and indirect
    //by freeing the data for all blocks
    for (int i = 0; i < NUM_DIRECT_INODE_BLOCKS; i++) {
        if (fileInode.direct_blocks[i] != 0) {
            free_data_block(fileInode.direct_blocks[i]);
        }
    }
    if (fileInode.indirect_block != 0) {
        int indirect_blocks[NUM_SINGLE_INDIRECT_BLOCKS];
        if (read_sd_block(indirect_blocks, fileInode.indirect_block)) {
            for (int i = 0; i < NUM_SINGLE_INDIRECT_BLOCKS; i++) {
                if (indirect_blocks[i] != 0) {
                    free_data_block(indirect_blocks[i]);
                }
            }
        }
        free_data_block(fileInode.indirect_block);
    }

    //Now that we have freed all the datablocks associated, we need to free the inode
    free_inode(inode_number);

    //Now that the inode was freed, we need to remove the directory entry
    remove_dir_entry(name);

    //We have successfully removed the file
    fserror = FS_NONE;
    return 1;
}

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name) {
    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return 0;
    }

    //Now that we have confirmed the name is valid
    //We can try to find a directory that has the same name 
    int dir_index = find_name_dir_entry(name);
    //if we have an index that is valid, we have a directory under the same
    //name so the file exists
    if (dir_index != -1) {
        fserror = FS_NONE;
        return 1;
    }
    else {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }
}

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void) {
    //Here we are going to check and print based off of the errors
    if (fserror == FS_EXCEEDS_MAX_FILE_SIZE) {
        printf("File has exceeded maximum size.\n");
    }
    else if (fserror == FS_FILE_ALREADY_EXISTS) {
        printf("The file alreaady exists.\n");
    }
    else if (fserror == FS_FILE_NOT_FOUND) {
        printf("The file was not found.\n");
    }
    else if (fserror == FS_FILE_NOT_OPEN) {
        printf("The file is not open.\n");
    }
    else if (fserror == FS_FILE_OPEN) {
        printf("The file is already open.\n");
    }
    else if (fserror == FS_FILE_READ_ONLY) {
        printf("The file is read only access.\n");
    }
    else if (fserror == FS_ILLEGAL_FILENAME) {
        printf("The file has an illegal file name.\n");
    }
    else if (fserror == FS_IO_ERROR) {
        printf("The file has an I/O error.\n");
    }
    else if (fserror == FS_OUT_OF_SPACE) {
        printf("The disk is out of space.\n");
    }
    else if (fserror == FS_NONE) {
        printf("There is no error.\n");
    }
    else {
        printf("Unknown error, something went wrong.\n");
    }
}

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void) {

    //Check to make sure the inode size fits in the blocks correctly
    if (sizeof(Inode) * INODES_PER_BLOCK != SOFTWARE_DISK_BLOCK_SIZE) {
        return 0;
    }

    //Check to make sure the directory size fits in the blocks correctly
    if (sizeof(DirEntry) * DIR_ENTRIES_PER_BLOCK != SOFTWARE_DISK_BLOCK_SIZE) {
        return 0;
    }

    return 1;
}

// Directory Entry Functions**********

// find a directory entry
// returns index of the free entry if found, -1 if not
int find_free_dir_entry() {

  for (int block = FIRST_DIR_ENTRY_BLOCK; block <= LAST_DIR_ENTRY_BLOCK; block++) {
    DirEntry entries[DIR_ENTRIES_PER_BLOCK];
    read_sd_block(entries, block);

    for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
      if (!entries[i].is_valid) {
        return (block - FIRST_DIR_ENTRY_BLOCK) * DIR_ENTRIES_PER_BLOCK + i;
      }
    }
  }

  fserror = FS_OUT_OF_SPACE;
  return -1;
}

// search for a directory entry by name
// returns index of the entry if found, -1 if not
int find_name_dir_entry(const char *name) {
  int block = FIRST_DIR_ENTRY_BLOCK;
  DirEntry entries[DIR_ENTRIES_PER_BLOCK];
  read_sd_block(entries, block);

  for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
    if (entries[i].is_valid && strcmp(entries[i].name, name) == 0) {
      return (block - FIRST_DIR_ENTRY_BLOCK) * DIR_ENTRIES_PER_BLOCK + i;
    }
  }

  fserror = FS_FILE_NOT_FOUND;
  return -1;
}

// gets index of searched file
// returns 1 if success, 0 if not
int get_dir_entry(int index, DirEntry *entry) {
    // Check if index is valid
    if (index < 0 || index >= (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK + 1) * DIR_ENTRIES_PER_BLOCK) {
        fserror = FS_IO_ERROR;
        return 0;
    }
    
    // Calculate block and offset
    int block = FIRST_DIR_ENTRY_BLOCK + index / DIR_ENTRIES_PER_BLOCK;
    int offset = index % DIR_ENTRIES_PER_BLOCK;
    
    // Read the block
    DirEntry entries[DIR_ENTRIES_PER_BLOCK];
    read_sd_block(entries, block);
    
    // Copy the entry to the parameter
    *entry = entries[offset];
    return 1;
}

// add a directory entry
// returns 1 if success, 0 if not
int add_dir_entry(char *name, unsigned long inode_index) {

    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return 0;
    }

    //Next we need to check to see if the inode is valid
    int total_inodes = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;
    
    if (inode_index < 0 || inode_index >= total_inodes) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we need to check if the file already exists
    if (find_name_dir_entry(name) != -1) {
        fserror = FS_FILE_ALREADY_EXISTS;
        return 0;
    }

    //Now we need to find the free directory to place the new one
    int dir_index = find_free_dir_entry();
    //If the index is negative 1, we have no more room to place the directory
    if (dir_index == -1) {
        fserror = FS_OUT_OF_SPACE;
        return 0;
    }

    //find the block of directories and the offset where we need to place the new one
    int blk = FIRST_DIR_ENTRY_BLOCK + (dir_index / DIR_ENTRIES_PER_BLOCK);
    int off = dir_index % DIR_ENTRIES_PER_BLOCK;

    //Now we need to read the entire block so we can place our directory into the block
    DirEntry dir_block[DIR_ENTRIES_PER_BLOCK];
    //Add the if so if the read fails we add the error
    if (!read_sd_block(dir_block, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we need to place the information in our new directory
    strncpy(dir_block[off].name, name, MAX_FILENAME_SIZE -1);
    dir_block[off].name[MAX_FILENAME_SIZE - 1] = '\0';
    dir_block[off].inode_number = inode_index;
    dir_block[off].is_valid = 1;

    //Now we need to write our new directory entry back into the block
    //nest in an if to catch if the write fails
    if (!write_sd_block(dir_block, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we have successfully made it through the add, so we return no error and 1
    fserror = FS_NONE;
    return 1;
}

// removes a directory entry
// returns 1 if success, 0 if not
int remove_dir_entry(const char *name) {
    //First we need to check to make sure that the name is valid
    if (name == NULL || strlen(name) == 0 || strlen(name) > MAX_FILENAME_SIZE) {
        fserror = FS_ILLEGAL_FILENAME;
        return 0;
    }

    //Now we know the name is valid, so we need to get the index of the directory
    int dir_index = find_name_dir_entry(name);
    //Check to make sure the file was found
    if (dir_index == -1) {
        fserror = FS_FILE_NOT_FOUND;
        return 0;
    }

    //find the block of directories and the offset where we need to remove the old
    int blk = FIRST_DIR_ENTRY_BLOCK + (dir_index / DIR_ENTRIES_PER_BLOCK);
    int off = dir_index % DIR_ENTRIES_PER_BLOCK;

    //Now we need to read the entire block so we can remove the old directory
    DirEntry dir_block[DIR_ENTRIES_PER_BLOCK];
    //Add the if so if the read fails we add the error
    if (!read_sd_block(dir_block, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we are going to take our entry and make it not valid
    dir_block[off].is_valid = 0;
    dir_block[off].name[0] = '\0';
    dir_block[off].inode_number = 0;

    //Now we write the entry that has been made invalid back to the block
    //nest in an if to catch if the write fails
    if (!write_sd_block(dir_block, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we have successfully made it through the add, so we return no error and 1
    fserror = FS_NONE;
    return 1;
}

// Inode Functions

// Find a free inode
// returns inode number if success, 0 if not
int find_free_inode() {
    unsigned char inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);

    int total_inodes = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;

    for (int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++) {
        // Check each byte for a free bit
        if (inode_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                // If the bit is not set, we found a free inode
                if (!(inode_bitmap[i] & (1 << j))) {
                    int inode_num = i * 8 + j;
                    if (inode_num < total_inodes) {  // Make sure we're within valid range
                        fserror = FS_NONE;
                        return inode_num;
                    }
                }
            }
        }
    }
    fserror = FS_OUT_OF_SPACE;
    return 0;
}

// Allocate an inode
// returns 1 if success, 0 if not
int allocate_inode(int inode_num) {
    int total_inodes = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;
    
    if (inode_num < 0 || inode_num >= total_inodes) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    unsigned char inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);

    // Calculate byte and bit position in bitmap
    int byte_index = inode_num / 8;
    int bit_index = inode_num % 8;

    // if allocated already return an error
    if (inode_bitmap[byte_index] & (1 << bit_index)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    inode_bitmap[byte_index] |= (1 << bit_index);
    write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);

    fserror = FS_NONE;
    return 1;
}

// Free an inode
// Returns 1 on success, 0 on failure
int free_inode(int inode_num) {
    int total_inodes = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;
    
    if (inode_num < 0 || inode_num >= total_inodes) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    unsigned char inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);

    // Calculate byte and bit position in bitmap
    int byte_index = inode_num / 8;
    int bit_index = inode_num % 8;


    // Check if block is already free
    if (!(inode_bitmap[byte_index] & (1 << bit_index))) {
        fserror = FS_NONE;  // Not an error, just already free
        return 1;  // Return success since the block is already free
    }

    // Clear the bit to mark the block as free
    inode_bitmap[byte_index] &= ~(1 << bit_index);
    write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK);

    fserror = FS_NONE;
    return 1;

}

// Get an inode by number
// Returns 1 on success, 0 on failure
int get_inode(int inode_num, Inode *inode) {
    
    //First we have to find the total number of inodes
    int total = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;

    //Now we check if our inode number is valid and in range
    if (inode_num < 0 || inode_num >= total) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we find the block that our inode is in, and the offset of where it is
    //in the block
    int blk = FIRST_INODE_BLOCK + (inode_num / INODES_PER_BLOCK);
    int off = inode_num % INODES_PER_BLOCK;

    Inode blk_inodes[INODES_PER_BLOCK];

    if (!read_sd_block(blk_inodes, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we copy the inode at the offset to the output 
    //This copies our inode from the block at the offset and places
    //it into the output pointer so we have "gotten" our inode
    *inode = blk_inodes[off];

    fserror = FS_NONE;
    return 1;
}


// Write an inode by number
// Returns 1 on success, 0 on failure
int write_inode(int inode_num, const Inode *inode) {

    //First we find the total number of inodes that we can have
    int total = (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1) * INODES_PER_BLOCK;

    //Now we check if our inode number is valid and it fits in our range
    if (inode_num < 0 || inode_num >= total) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we find the block that our inode is in, and the offset of where it is
    //in the block
    int blk = FIRST_INODE_BLOCK + (inode_num / INODES_PER_BLOCK);
    int off = inode_num % INODES_PER_BLOCK;

    //After that, we create a new variable blk_inodes of all the inodes per block
    Inode blk_inodes[INODES_PER_BLOCK];
    
    //Now we attempt to read the sd block in our block and the inodes in that block
    //if we don'd find, we set the error
    if (!read_sd_block(blk_inodes, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    //Now we replace the inode at the offset in our block of inodes to the inode from 
    //our parameters
    blk_inodes[off] = *inode;

    //lastly we perform the write using sd to write all of the blk_inodes back to the block
    //after we just replaced our inode. If this fails, we catch using the if and give an error
    if (!write_sd_block(blk_inodes, blk)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    fserror = FS_NONE;
    return 1;

}

// Data Block Functions

// Find a free data block
// Returns the index of a free data block, or 0 if none are available
int find_free_data_block() {
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);

    for (int i = 0; i < SOFTWARE_DISK_BLOCK_SIZE; i++) {
        // I/O measures 8 bits at a time, so I have to look for each bit individually
        // with another for loop
        if (data_bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                // So if the data bitmap[i] = 1011111, j is going to repeat until it hits the 0,
                if (!(data_bitmap[i] & (1 << j))) {
                    fserror = FS_NONE;
                    return (i * 8 + j) + FIRST_DATA_BLOCK;
                }
            }
        }
    }
    fserror = FS_OUT_OF_SPACE;
    return 0;
}

// Allocate a data block
// Returns 1 on success, 0 on failure
int allocate_data_block(int block_num) {
    if (block_num < FIRST_DATA_BLOCK || block_num > LAST_DATA_BLOCK) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);

    // Calculate relative position from start of data blocks
    int relative_block = block_num - FIRST_DATA_BLOCK;
    int byte_index = relative_block / 8;
    int bit_index = relative_block % 8;

    // if allocated already return an error
    if (data_bitmap[byte_index] & (1 << bit_index)) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    data_bitmap[byte_index] |= (1 << bit_index);
    write_sd_block(data_bitmap, DATA_BITMAP_BLOCK);

    fserror = FS_NONE;
    return 1;
}

// Free a data block
// Returns 1 on success, 0 on failure
int free_data_block(int block_num) {
    if (block_num < FIRST_DATA_BLOCK || block_num > LAST_DATA_BLOCK) {
        fserror = FS_IO_ERROR;
        return 0;
    }

    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);

    // Calculate relative position from start of data blocks
    int relative_block = block_num - FIRST_DATA_BLOCK;
    int byte_index = relative_block / 8;
    int bit_index = relative_block % 8;

    // Check if block is already free
    if (!(data_bitmap[byte_index] & (1 << bit_index))) {
        fserror = FS_NONE;  // Not an error, just already free
        return 1;  // Return success since the block is already free
    }

    // Clear the bit to mark the block as free
    data_bitmap[byte_index] &= ~(1 << bit_index);
    write_sd_block(data_bitmap, DATA_BITMAP_BLOCK);

    fserror = FS_NONE;
    return 1;
}

// Just for clarification on when I use Superblock
int write_superblock(const Superblock *sb) {
    return write_sd_block((void*)sb, SUPERBLOCK_BLOCK);
}

int read_superblock(Superblock *sb) {
    return read_sd_block((void*)sb, SUPERBLOCK_BLOCK); 
}
