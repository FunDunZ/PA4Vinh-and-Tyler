//
// Simple filesystem API for LSU 4103 filesystem assignment.
// (@nolaforensix), 11/2017.  Minor updates 11/2019.  Updated 6/2022.
// Modified by AAG, 11/2022

#if ! defined(__FILESYSTEM_4103_H__)
#define __FILESYSTEM_4103_H__

#include <stdint.h>

#define MAX_FILES 512
#define SUPERBLOCK_BLOCK 0
#define DATA_BITMAP_BLOCK 1
#define INODE_BITMAP_BLOCK 2
#define FIRST_INODE_BLOCK 3
#define LAST_INODE_BLOCK 6 // 128 inodes per block, max of 4*128 = 512 inodes, thus 512 files

#define INODES_PER_BLOCK 128
#define FIRST_DIR_ENTRY_BLOCK 7
#define LAST_DIR_ENTRY_BLOCK 70
#define DIR_ENTRIES_PER_BLOCK 8 // 8 DE per block, max of 64*8 = 512 Directory entries

#define FIRST_DATA_BLOCK 71
#define LAST_DATA_BLOCK 4095
#define MAX_FILENAME_SIZE 507
#define NUM_DIRECT_INODE_BLOCKS 13
#define NUM_SINGLE_INDIRECT_BLOCKS (SOFTWARE_DISK_BLOCK_SIZE / sizeof(uint16_t))

#define MAX_FILE_SIZE ((NUM_DIRECT_INODE_BLOCKS + NUM_SINGLE_INDIRECT_BLOCKS) * SOFTWARE_DISK_BLOCK_SIZE)


// private
struct FileInternals;


// file type used by user code
typedef struct FileInternals* File;

// Superblock structure
typedef struct Superblock{
    unsigned long total_blocks;
    unsigned long free_blocks;
    unsigned long total_inodes;
    unsigned long free_inodes;
    unsigned long block_size;
    unsigned long inode_size;
    unsigned long block_bitmap_start;
    unsigned long inode_bitmap_start;
    unsigned long inode_table_start;
    unsigned long data_blocks_start;
} Superblock;

// Inode structure
typedef struct Inode {
    unsigned long inode_number;
    unsigned long file_size;
    uint16_t direct_blocks[NUM_DIRECT_INODE_BLOCKS];
    uint16_t indirect_block;
} Inode;

// Directory entry structure
typedef struct DirEntry {
    char name[MAX_FILENAME_SIZE];
    unsigned long inode_number;
    unsigned char is_valid;  // 1 if this entry is valid, 0 if free
} DirEntry;

// access mode for open_file() 
typedef enum {
	READ_ONLY, READ_WRITE
} FileMode;

// error codes set in global 'fserror' by filesystem functions
typedef enum  {
  FS_NONE, 
  FS_OUT_OF_SPACE,         // the operation caused the software disk to fill up
  FS_FILE_NOT_OPEN,  	   // attempted read/write/close/etc. on file that isn't open
  FS_FILE_OPEN,      	   // file is already open. Concurrent opens are not
                           // supported and neither is deleting a file that is open.
  FS_FILE_NOT_FOUND, 	   // attempted open or delete of file that doesn't exist
  FS_FILE_READ_ONLY, 	   // attempted write to file opened for READ_ONLY
  FS_FILE_ALREADY_EXISTS,  // attempted creation of file with existing name
  FS_EXCEEDS_MAX_FILE_SIZE,// seek or write would exceed max file size
  FS_ILLEGAL_FILENAME,     // filename begins with a null character
  FS_IO_ERROR              // something really bad happened
} FSError;

// function prototypes for filesystem API

// open existing file with pathname 'name' and access mode 'mode'.
// Current file position is set to byte 0.  Returns NULL on
// error. Always sets 'fserror' global.
File open_file(char *name, FileMode mode);

// create and open new file with pathname 'name' and (implied) access
// mode READ_WRITE.  Current file position is set to byte 0.  Returns
// NULL on error. Always sets 'fserror' global.
File create_file(char *name);

// close 'file'.  Always sets 'fserror' global.
void close_file(File file);

// read at most 'numbytes' of data from 'file' into 'buf', starting at the 
// current file position.  Returns the number of bytes read. If end of file is reached,
// then a return value less than 'numbytes' signals this condition. Always sets
// 'fserror' global.
unsigned long read_file(File file, void *buf, unsigned long numbytes);

// write 'numbytes' of data from 'buf' into 'file' at the current file
// position.  Returns the number of bytes written. On an out of space
// error, the return value may be less than 'numbytes'.  Always sets
// 'fserror' global.
unsigned long write_file(File file, void *buf, unsigned long numbytes);

// sets current position in file to 'bytepos', always relative to the
// beginning of file.  Seeks past the current end of file should
// extend the file. Returns 1 on success and 0 on failure.  Always
// sets 'fserror' global.
int seek_file(File file, unsigned long bytepos);

// returns the current length of the file in bytes. Always sets
// 'fserror' global.
unsigned long file_length(File file);

// deletes the file named 'name', if it exists. Returns 1 on success,
// 0 on failure.  Always sets 'fserror' global.
int delete_file(char *name); 

// determines if a file with 'name' exists and returns 1 if it exists, otherwise 0.
// Always sets 'fserror' global.
int file_exists(char *name);

// describe current filesystem error code by printing a descriptive
// message to standard error.
void fs_print_error(void);

// extra function to make sure structure alignment, data structure
// sizes, etc. on target platform are correct.  Should return 1 on
// success, 0 on failure.  This should be used in disk initialization
// to ensure that everything will work correctly.
int check_structure_alignment(void);


// Directory Entry Functions**********

// find a directory entry
// returns 1 if success, 0 if not
int find_free_dir_entry();

// search for a directory entry by name
// returns 1 if success, 0 if not
int find_name_dir_entry(const char *name);

// gets index of searched file
// returns 1 if success, 0 if not
int get_dir_entry(int index, DirEntry *entry);

// add a directory entry
// returns 1 if success, 0 if not
int add_dir_entry(char *name, unsigned long inode_index);

// removes a directory entry
// returns 1 if success, 0 if not
int remove_dir_entry(const char *name);

// Inode Functions

// Find a free inode
// returns 1 if success, 0 if not
int find_free_inode();

// Allocate an inode
// returns 1 if success, 0 if not
int allocate_inode(int inode_num);

// Free an inode
// Returns 1 on success, 0 on failure
int free_inode(int inode_num);

// Get an inode by number
// Returns 1 on success, 0 on failure
int get_inode(int inode_num, Inode *inode);

// Write an inode by number
// Returns 1 on success, 0 on failure
int write_inode(int inode_num, const Inode *inode);

// Data Block Functions

// Find a free data block
// Returns the index of a free data block, or -1 if none are available
int find_free_data_block();

// Allocate a data block
// Returns 1 on success, 0 on failure
int allocate_data_block(int block_num);

// Free a data block
// Returns 1 on success, 0 on failure
int free_data_block(int block_num);

// ETC

// filesystem error code set (set by each filesystem function)
extern FSError fserror;

// Superblock functions
int write_superblock(const Superblock *sb);
int read_superblock(Superblock *sb);


#endif
