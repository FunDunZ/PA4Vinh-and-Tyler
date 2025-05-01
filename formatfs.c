#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"
#include "filesystem.c"
#include "softwaredisk.h"
#include "softwaredisk.c"

int main() {
    if (!init_software_disk()) {
        printf("Failed to initialize software disk.\n");
        sd_print_error();
        return 1;
    }
    
    // Initialize the superblock
    Superblock sb;
    sb.total_blocks = software_disk_size();
    sb.total_inodes = MAX_FILES;
    sb.free_inodes = sb.total_inodes;
    sb.block_size = SOFTWARE_DISK_BLOCK_SIZE;
    sb.inode_size = sizeof(Inode);
    sb.block_bitmap_start = DATA_BITMAP_BLOCK;
    sb.inode_bitmap_start = INODE_BITMAP_BLOCK;
    sb.inode_table_start = FIRST_INODE_BLOCK;
    sb.data_blocks_start = FIRST_DATA_BLOCK;
    
    // Calculate free blocks (all data blocks are initially free)
    sb.free_blocks = LAST_DATA_BLOCK - FIRST_DATA_BLOCK + 1;
    
    // Write the superblock
    if (!write_superblock(&sb)) {
        printf("Failed to write superblock.\n");
        return 1;
    }
    
    // Initialize data block bitmap
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    memset(data_bitmap, 0, SOFTWARE_DISK_BLOCK_SIZE); // All bits set to 0 (free)
    if (!write_sd_block(data_bitmap, DATA_BITMAP_BLOCK)) {
        printf("Failed to initialize data block bitmap.\n");
        return 1;
    }
    
    // Initialize inode bitmap
    unsigned char inode_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    memset(inode_bitmap, 0, SOFTWARE_DISK_BLOCK_SIZE); // All bits set to 0 (free)
    if (!write_sd_block(inode_bitmap, INODE_BITMAP_BLOCK)) {
        printf("Failed to initialize inode bitmap.\n");
        return 1;
    }
    
    // Initialize inode table
    unsigned char inode_table[SOFTWARE_DISK_BLOCK_SIZE * (LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1)];
    memset(inode_table, 0, sizeof(inode_table)); // All inodes set to 0
    for (int i = 0; i < LAST_INODE_BLOCK - FIRST_INODE_BLOCK + 1; i++) {
        if (!write_sd_block(inode_table + i * SOFTWARE_DISK_BLOCK_SIZE, FIRST_INODE_BLOCK + i)) {
            printf("Failed to initialize inode table.\n");
            return 1;
        }
    }
    
    // Initialize directory entries
    unsigned char dir_entries[SOFTWARE_DISK_BLOCK_SIZE * (LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK + 1)];
    memset(dir_entries, 0, sizeof(dir_entries)); // All directory entries set to 0
    for (int i = 0; i < LAST_DIR_ENTRY_BLOCK - FIRST_DIR_ENTRY_BLOCK + 1; i++) {
        if (!write_sd_block(dir_entries + i * SOFTWARE_DISK_BLOCK_SIZE, FIRST_DIR_ENTRY_BLOCK + i)) {
            printf("Failed to initialize directory entries.\n");
            return 1;
        }
    }
    
    printf("Filesystem formatted successfully.\n");
    return 0;
}
