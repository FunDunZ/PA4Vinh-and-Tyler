#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../filesystem.h"
#include "../softwaredisk.h"


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
  
}
