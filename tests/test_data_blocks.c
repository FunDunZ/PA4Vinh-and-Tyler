#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../filesystem.h"
#include "../softwaredisk.h"

// Declare global variables
extern FSError fserror;
extern SDError sderror;

void test_find_free_data_block() {
    printf("Testing find_free_data_block()...\n");
    
    // Initialize software disk
    if (!init_software_disk()) {
        printf("Failed to initialize software disk\n");
        return;
    }
    
    // Set up test bitmap - only block 0 is allocated
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    memset(data_bitmap, 0, SOFTWARE_DISK_BLOCK_SIZE);
    data_bitmap[0] = 0x01;  // First bit set (block 0 allocated)
    
    // Write bitmap to disk
    write_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    
    // Test find_free_data_block
    int free_block = find_free_data_block();
    if (free_block == FIRST_DATA_BLOCK + 1) {
        printf("Test passed: Found correct free block\n");
    } else {
        printf("Test failed: Expected block %d, got %d\n", FIRST_DATA_BLOCK + 1, free_block);
    }
}

void test_allocate_data_block() {
    printf("\nTesting allocate_data_block()...\n");
    
    // Test allocating a valid free block
    int block_to_allocate = FIRST_DATA_BLOCK + 1;
    if (allocate_data_block(block_to_allocate)) {
        printf("Test passed: Successfully allocated block %d\n", block_to_allocate);
    } else {
        printf("Test failed: Failed to allocate block %d\n", block_to_allocate);
    }
    
    // Verify the block is now allocated
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    int byte_index = 0;
    int bit_index = 1;  // Second bit (block 1)
    
    if (data_bitmap[byte_index] & (1 << bit_index)) {
        printf("Test passed: Block %d is now marked as allocated\n", block_to_allocate);
    } else {
        printf("Test failed: Block %d is not marked as allocated\n", block_to_allocate);
    }
    
    // Test allocating an already allocated block
    if (!allocate_data_block(block_to_allocate)) {
        printf("Test passed: Correctly rejected allocating already allocated block\n");
    } else {
        printf("Test failed: Should have rejected allocating already allocated block\n");
    }
    
    // Test allocating an invalid block number
    if (!allocate_data_block(FIRST_DATA_BLOCK - 1)) {
        printf("Test passed: Correctly rejected invalid block number\n");
    } else {
        printf("Test failed: Should have rejected invalid block number\n");
    }
}

void test_free_data_block() {
    printf("\nTesting free_data_block()...\n");
    
    // First allocate a block to free
    int block_to_free = FIRST_DATA_BLOCK + 1;
    allocate_data_block(block_to_free);
    
    // Test freeing an allocated block
    if (free_data_block(block_to_free)) {
        printf("Test passed: Successfully freed block %d\n", block_to_free);
    } else {
        printf("Test failed: Failed to free block %d\n", block_to_free);
    }
    
    // Verify the block is now free
    unsigned char data_bitmap[SOFTWARE_DISK_BLOCK_SIZE];
    read_sd_block(data_bitmap, DATA_BITMAP_BLOCK);
    int byte_index = 0;
    int bit_index = 1;  // Second bit (block 1)
    
    if (!(data_bitmap[byte_index] & (1 << bit_index))) {
        printf("Test passed: Block %d is now marked as free\n", block_to_free);
    } else {
        printf("Test failed: Block %d is still marked as allocated\n", block_to_free);
    }
    
    // Test freeing an already free block
    if (free_data_block(block_to_free)) {
        printf("Test passed: Correctly handled freeing already free block\n");
    } else {
        printf("Test failed: Should have handled freeing already free block\n");
    }
    
    // Test freeing an invalid block number
    if (!free_data_block(FIRST_DATA_BLOCK - 1)) {
        printf("Test passed: Correctly rejected invalid block number\n");
    } else {
        printf("Test failed: Should have rejected invalid block number\n");
    }
}

int main() {
    test_find_free_data_block();
    test_allocate_data_block();
    test_free_data_block();
    return 0;
} 