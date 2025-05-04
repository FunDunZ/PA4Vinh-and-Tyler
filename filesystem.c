#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "softwaredisk.h"
#include "filesystem.h"

// Define the global error variable
FSError fserror = FS_NONE;

struct FileInternals {
    int inode_index;            // index in inode table
    FileMode user_mode;
    unsigned long position;    // current position in file
    int d;                      

};


// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode) {
    // TODO: implement

    return NULL;
}

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name) {

    // TODO: implement
    return NULL;
}

// close 'file'.  Always sets 'fserror' global.
void close_file(File file) {
    // TODO: implement
}

// read at most 'numbytes' of data from 'file' into 'buf', starting at the 
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes) {
    // TODO: implement
    return 0;
}

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes) {
    // TODO: implement
    return 0;
}

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos) {
    // TODO: implement
    return 0;
}

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file) {
    // TODO: implement
    return 0;
}

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name) {
    // TODO: implement
    return 0;
}

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name) {
    // TODO: implement
    return 0;
}

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void) {
    // TODO: implement
}

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void) {
    // TODO: implement
    return 1;
}

// Directory Entry Functions**********

// find a directory entry
// returns index of the free entry if found, -1 if not
int find_free_dir_entry() {
  int block = FIRST_DIR_ENTRY_BLOCK;
  for (block; block <= LAST_DIR_ENTRY_BLOCK; block++) {
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
int find_name_dir_entry(char *name) {
  int block = FIRST_DIR_ENTRY_BLOCK;
  DirEntry entries[DIR_ENTRIES_PER_BLOCK];
  read_sd_block(entries, block);

  for (int i = 0; i < DIR_ENTRIES_PER_BLOCK; i++) {
    if (entries[i].is_valid && (entries[i].name, name) == 0) {
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
int add_dir_entry(char *name, unsigned long inode_index);
    // TODO: implement

// removes a directory entry
// returns 1 if success, 0 if not
int remove_dir_entry(const char *name);
    // TODO: implement

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
