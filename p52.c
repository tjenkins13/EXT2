/*
Author: Tyler Jenkins
Date: 10/9/17
 */

#include "ext2.h"

int main(int argc,char **argv)
{
   int fd;
   int rv;
   SUPERBLOCK sb;
   BGDT bg;
   INODE in,in2;
   DIR_STRUCT dir;
   time_t t;
   struct tm *tp;   // time pointer
   int i;
   unsigned char inode_bitmap[K];
   int pos=0;
   int block=0;
   
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
   lseek(fd,K,SEEK_SET); //go to second block- where superblock is
   rv = read(fd, &sb, sizeof(SUPERBLOCK));  //read superblock
   if(rv != sizeof(SUPERBLOCK)){
     fprintf(stderr,"bad superblock read\n");
     exit(0);
   }

   rv = read(fd, &bg, sizeof(BGDT));  //read block group descriptor table
   if(rv != sizeof(BGDT)){
     fprintf(stderr,"bad block group read\n");
     exit(0);
   }

   lseek(fd,K*bg.bg_inode_bitmap,SEEK_SET); //go to where inode bitmap is

   rv=read(fd,&inode_bitmap,sb.s_inodes_count);
   if(rv!=sb.s_inodes_count){
     fprintf(stderr,"bad inode table read\n");
     exit(0);
   }

   lseek(fd,K*bg.bg_inode_table,SEEK_SET); //go to where inode table is
   lseek(fd,sizeof(INODE),SEEK_CUR); //go to where first used inode is
   rv=read(fd,&in,sizeof(INODE)); //read inode
   
   if(rv!=sizeof(INODE)){
     fprintf(stderr,"Bad inode read\n");
     exit(0);
   }

   /*   lseek(fd,K*in.i_block[0],SEEK_SET); //go to where block is
   
   rv=read(fd,&dir,sizeof(DIR_STRUCT)-255);
   
   if(rv!=sizeof(DIR_STRUCT)-255){
     fprintf(stderr,"Bad directory read\n");
     exit(0);
   }
   read(fd,&dir.name,dir.name_len);
   dir.name[dir.name_len]=0;
   printf("%s %d num blocks:%d\n",dir.name,dir.inode,in.i_blocks);
   pos=dir.rec_len;*/
   dir.inode=1;    
   while(dir.inode!=0 && block<=(in.i_blocks-1)){
     /*if(pos!=0)
       lseek(fd,dir.rec_len-((sizeof(DIR_STRUCT)-255)+dir.name_len),SEEK_CUR);*/
     lseek(fd,(K*in.i_block[block])+pos,SEEK_SET);
     rv=read(fd,&dir,sizeof(DIR_STRUCT)-255);
   
     if(rv!=sizeof(DIR_STRUCT)-255){
       fprintf(stderr,"Bad directory read\n");
       exit(0);
     }
     read(fd,&dir.name,dir.name_len);
     dir.name[dir.name_len]=0;

     if(dir.inode==0)
       break;
     
     lseek(fd,(K*bg.bg_inode_table)+((dir.inode-1)*sizeof(INODE)),SEEK_SET);

     rv=read(fd,&in2,sizeof(INODE));
     if(rv!=sizeof(INODE)){
       fprintf(stderr,"Bad Inode read\n");
       exit(0);
     }
     printmode(in2);
     printf(" %2d %5s %5s %5d ",in2.i_links_count,
	    getpwuid((uid_t)in2.i_uid)->pw_name,
	    getgrgid((gid_t)in2.i_gid)->gr_name,
	    in2.i_size);
     
     printdate(in2);
     printf(" %s\n",dir.name);

     pos+=dir.rec_len;
     if((pos+(sizeof(DIR_STRUCT)-255))>=K){ //went too far go to next block
       block++;
       pos=0;
       lseek(fd,in.i_block[block]*K,SEEK_SET);
       //printf("Done in block\n");
     }

   }

   close(fd);
}
