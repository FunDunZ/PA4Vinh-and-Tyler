#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "softwaredisk.h"
#include "filesystem.h"

struct FileInternals {
    int inode_index;            // index in inode table
    FileMode user_mode;
    unsigned long position;    // current position in file

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

    if (!read_sd_block(entries, block)) {
      fserror = FS_IO_ERROR;
      return -1;
    }

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

  if (!read_sd_block(entries, block)) {
    fserror = FS_IO_ERROR;
    return -1;
  }

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
    if (!read_sd_block(entries, block)) {
        fserror = FS_IO_ERROR;
        return 0;
    }
    
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
// returns 1 if success, 0 if not
int find_free_inode();
    // TODO: implement

// Allocate an inode
// returns 1 if success, 0 if not
int allocate_inode(int inode_num);
    // TODO: implement

// Free an inode
// Returns 1 on success, 0 on failure
int free_inode(int inode_num);
    // TODO: implement

// Get an inode by number
// Returns 1 on success, 0 on failure
int get_inode(int inode_num, Inode *inode);
    // TODO: implement

// Write an inode by number
// Returns 1 on success, 0 on failure
int write_inode(int inode_num, const Inode *inode);
    // TODO: implement

// Data Block Functions

// Find a free data block
// Returns the index of a free data block, or -1 if none are available
int find_free_data_block();
    // TODO: implement

// Allocate a data block
// Returns 1 on success, 0 on failure
int allocate_data_block(int block_num);
    // TODO: implement

// Free a data block
// Returns 1 on success, 0 on failure
int free_data_block(int block_num);
    // TODO: implement



// Just for clarification on when I use Superblock
int write_superblock(const Superblock *sb) {
    return write_sd_block((void*)sb, SUPERBLOCK_BLOCK);
}

int read_superblock(Superblock *sb) {
    return read_sd_block((void*)sb, SUPERBLOCK_BLOCK); 
}
