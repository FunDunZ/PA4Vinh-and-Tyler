#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "softwaredisk.c"
#include "filesystem.c"

char poetry[]="WORK PLEASEEEEEEEEEEEE";

int main(int argc, char *argv[]) {
  char buf[SOFTWARE_DISK_BLOCK_SIZE];
  int i,j, ret;

  init_software_disk();
  printf("Size of software disk in blocks: %lu, block size = %d\n", software_disk_size(), SOFTWARE_DISK_BLOCK_SIZE);
  sd_print_error();
  
  printf("Writing a block of A's to block # 3.\n");
  memset(buf, 'A', SOFTWARE_DISK_BLOCK_SIZE);
  ret=write_sd_block(buf, 3);
  printf("Return value was %d.\n", ret);
  sd_print_error();
  printf("Reading block # 3.\n");
  bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
  ret=read_sd_block(buf, 3);
  printf("Return value was %d.\n", ret);
  sd_print_error();
  printf("Contents of block # 3:\n");
  for (j=0; j < SOFTWARE_DISK_BLOCK_SIZE; j++) {
    printf("%c", buf[j]);
  }
  printf("\n");
  
  printf("Reading block # 7.\n");
  bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
  ret=read_sd_block(buf, 7);
  printf("Return value was %d.\n", ret);
  sd_print_error();
  printf("Contents of block # 7 in hex:\n");
  for (j=0; j < SOFTWARE_DISK_BLOCK_SIZE; j++) {
    printf("0x%02x ", buf[j]);
  }
  printf("\n");
  
  printf("Trying to write a block of B's to block # %lu (should fail).\n", software_disk_size());
  memset(buf, 'B', SOFTWARE_DISK_BLOCK_SIZE);
  ret=write_sd_block(buf, software_disk_size());
  printf("Return value was %d.\n", ret);
  sd_print_error();

  printf("Trying to write a block of B's to block # %lu (should succeed).\n", software_disk_size() - 1);
  ret=write_sd_block(buf, software_disk_size() - 1);
  printf("Return value was %d.\n", ret);
  sd_print_error();
  
  printf("Writing some poetry to blocks #5-7.\n");
  bzero(buf, SOFTWARE_DISK_BLOCK_SIZE);
  memcpy(buf, poetry, strlen(poetry));
  for (i=5; i <= 7; i++) {
    ret=write_sd_block(buf, (unsigned long)i);
    printf("Return value was %d.\n", ret);
    sd_print_error();
  }
  printf("Reading blocks # 5-7.\n");
  for (i=5; i <= 7; i++) {
    ret=read_sd_block(buf, (unsigned long)i);
    printf("Return value was %d.\n", ret);
    sd_print_error();
    printf("Contents of block # %d:\n", i);
    for (j=0; j < SOFTWARE_DISK_BLOCK_SIZE; j++) {
      printf("%c", buf[j]);
    }
    printf("\n");
  }
}
