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
   INODE root,in;
   DIR_STRUCT dir;
   time_t t;
   struct tm *tp;   // time pointer
   int i;
   int block_size;
   int difsize;
   int pos;
   int bread;
   int block;
   int blocks[4]={0,0,0,0};
   
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
   /*printf("Number of blocks: %d\n",sb.s_blocks_count);
   printf("Number of inodes: %d\n",sb.s_inodes_count);
   printf("Number of reserved blocks: %d\n",sb.s_r_blocks_count);*/
   
   difsize=sb.s_inode_size-sizeof(INODE); //in case inode isn't 128 bytes
   if(difsize)
     fprintf(stderr,"Inode size is not %d\n",sizeof(INODE));
   block_size=1024 << sb.s_log_block_size;
   if(block_size>1024){
     lseek(fd,block_size*2,SEEK_SET);
   }
   
   rv=read(fd,&bg,sizeof(BGDT));//read block group descriptor table
   if(rv!=sizeof(BGDT)){
     fprintf(stderr,"Bad block group read\n");
     exit(0);
   }
   //printf("%d %d %d\n",bg.bg_block_bitmap,bg.bg_inode_bitmap,bg.bg_inode_table);

   lseek(fd,(block_size*bg.bg_inode_table)+sb.s_inode_size,SEEK_SET);
   rv=read(fd,&root,sizeof(INODE));
   if(rv!=sizeof(INODE)){
     fprintf(stderr,"Bad inode read\n");
     exit(0);
   }
   myls2(fd,root,block_size,sb,bg);

   ext2open(fd,"./mysubdir/file.txt",root,0,0);
   /*   dir.inode=1;
   pos=0;
   block=findblock(fd,root,blocks);
   
   while(dir.inode){
     lseek(fd,(block_size*block)+pos,SEEK_SET);
     rv=read(fd,&dir,sizeof(DIR_STRUCT)-255);
     if(rv!=(sizeof(DIR_STRUCT)-255)){
       fprintf(stderr,"Bad directory entry read\n");
       exit(0);
     }
     read(fd,&dir.name,dir.name_len);
     dir.name[dir.name_len]=0;
     pos+=dir.rec_len;
     
     if(dir.inode!=0){
       lseek(fd,(block_size*bg.bg_inode_table)+(sb.s_inode_size*(dir.inode-1)),SEEK_SET);
       rv=read(fd,&in,sizeof(INODE));
       if(rv!=sizeof(INODE)){
	 fprintf(stderr,"Bad inode read\n");
	 exit(0);
       }
       myls(fd,dir,in);

     }
     if(pos>=block_size){
       pos=0;
       block=findblock(fd,root,blocks);
     }
     }*/
   
   close(fd);
}
