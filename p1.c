/*
Author: Tyler Jenkins
Date: 10/16/17
 */

#include "ext2.h"

int main(int argc,char **argv)
{
   int fd;
   int rv;
   SUPERBLOCK sb;
   BGDT bg;
   INODE in;
   DIR_STRUCT dir;
   time_t t;
   struct tm *tp;   // time pointer
   int i;

   if(argc<2){
     fprintf(stderr,"usage: %s <disk>\n",argv[0]);
     exit(1);
   }
   
   //fprintf(stderr,"sizeof superblock = %ld\n", sizeof(SUPERBLOCK));
   fd = open(argv[1], O_RDONLY);
   if(fd < 0){
     fprintf(stderr,"cannot open %s\n",argv[1]);
     exit(0);
   }
   lseek(fd,1024,SEEK_SET); //go to second block- where superblock is
   rv = read(fd, &sb, sizeof(SUPERBLOCK));  //read superblock
   if(rv != sizeof(SUPERBLOCK)){
     fprintf(stderr,"bad superblock read\n");
     exit(0);
   }
   printf("Number of blocks: %d\n",sb.s_blocks_count);
   printf("Number of inodes: %d\n",sb.s_inodes_count);
   printf("Number of reserved blocks: %d\n",sb.s_r_blocks_count);
   printf("Block size: %d\n",K << sb.s_log_block_size);
   close(fd);
}
