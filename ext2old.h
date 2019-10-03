#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <grp.h>
#include <pwd.h>

//-- file format --
#define EXT2_S_IFSOCK   0xC000  //socket
#define EXT2_S_IFLNK    0xA000  //symbolic link
#define EXT2_S_IFREG    0x8000  //regular file
#define EXT2_S_IFBLK    0x6000  //block device
#define EXT2_S_IFDIR    0x4000  //directory
#define EXT2_S_IFCHR    0x2000  //character device
#define EXT2_S_IFIFO    0x1000  //fifo
//--process execution user/group override --
#define EXT2_S_ISUID    0x0800  //Set process User ID
#define EXT2_S_ISGID    0x0400  //Set process Group ID
#define EXT2_S_ISVTX    0x0200  //sticky bit
//-- access rights --
#define EXT2_S_IRUSR    0x0100  //user read
#define EXT2_S_IWUSR    0x0080  //user write
#define EXT2_S_IXUSR    0x0040  //user execute
#define EXT2_S_IRGRP    0x0020  //group read
#define EXT2_S_IWGRP    0x0010  //group write
#define EXT2_S_IXGRP    0x0008  //group execute
#define EXT2_S_IROTH    0x0004  //others read
#define EXT2_S_IWOTH    0x0002  //others write
#define EXT2_S_IXOTH    0x0001  //others execute

#define K 1024

typedef struct __attribute__((packed)) {
    unsigned int        s_inodes_count;
    unsigned int        s_blocks_count;
    unsigned int        s_r_blocks_count;
    unsigned int        s_free_blocks_count;
    unsigned int        s_free_inodes_count;
    unsigned int        s_first_data_block;
    unsigned int        s_log_block_size;
    unsigned int        s_log_frag_size;
    unsigned int        s_blocks_per_group;
    unsigned int        s_frags_per_group;
    unsigned int        s_inodes_per_group;
    unsigned int        s_mtime;
    unsigned int        s_wtime;
    unsigned short        s_mnt_count;
    unsigned short        s_max_mnt_count;
    unsigned short        s_magic;
    unsigned short        s_state;
    unsigned short        s_errors;
    unsigned short        s_minor_rev_level;
    unsigned int        s_lastcheck;
    unsigned int        s_checkinterval;
    unsigned int        s_creator_os;
    unsigned int        s_rev_level;
    unsigned short        s_def_resuid;
    unsigned short        s_def_resgid;
    unsigned int        s_first_ino;
    unsigned short        s_inode_size;
    unsigned short        s_block_group_nr;
    unsigned int        s_feature_compat;
    unsigned int        s_feature_incompat;
    unsigned int        s_feature_ro_compat;
    char      s_uuid[16];
    char      s_volume_name[16];
    char      s_last_mounted[64];
    char      unused[824];
}SUPERBLOCK;

typedef struct __attribute__((packed)) {
  unsigned int bg_block_bitmap;
  unsigned int bg_inode_bitmap;
  unsigned int bg_inode_table;
  unsigned short bg_free_blocks_count;
  unsigned short bg_free_inodes_count;
  unsigned short bg_used_dirs_count;
  unsigned short bg_pad;
  unsigned char bg_reserved[12];
  char unused[992];
} BGDT;

typedef struct __attribute__((packed)) {
  unsigned short i_mode;
  unsigned short i_uid;
  unsigned int i_size;
  unsigned int i_atime;
  unsigned int i_ctime;
  unsigned int i_mtime;
  unsigned int i_dtime;
  unsigned short i_gid;
  unsigned short i_links_count;
  unsigned int i_blocks;
  unsigned int i_flags;
  unsigned int _i_osd1;
  unsigned int i_block[15];
  unsigned int i_generation;
  unsigned int i_file_acl;
  unsigned int i_dir_acl;
  unsigned int i_faddr;
  unsigned char i_osd2[12]; //don't feel like using i_osd2 struct
} INODE;

typedef struct __attribute__((packed)) {
  unsigned int inode;
  unsigned short rec_len;
  unsigned char name_len;
  unsigned char file_type;
  unsigned char name[255];
} DIR_STRUCT;

char *months[12]={"Jan",
		  "Feb",
		  "Mar",
		  "Apr",
		  "May",
		  "Jun",
		  "Jul",
		  "Aug",
		  "Sep",
		  "Oct",
		  "Nov",
		  "Dec"};

void printmode(INODE in){

  if((in.i_mode & EXT2_S_IFDIR)>0)
    printf("d");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IRUSR)>0)
    printf("r");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IWUSR)>0)
    printf("w");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IXUSR)>0)
    printf("x");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IRGRP)>0)
    printf("r");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IWGRP)>0)
    printf("w");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IXGRP)>0)
    printf("x");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IROTH)>0)
    printf("r");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IWOTH)>0)
    printf("w");
  else
    printf("-");
  if((in.i_mode & EXT2_S_IXOTH)>0)
    printf("x");
  else
    printf("-");
}

void printdate(INODE in){
  struct tm *tp;
  time_t t=(time_t)in.i_atime;
  tp=localtime(&t);

  printf("%s %2d %2d:",months[tp->tm_mon],tp->tm_mday,tp->tm_hour);
  if(tp->tm_min<10)
    printf("0%d",tp->tm_min);
  else
    printf("%d",tp->tm_min);
    
}

unsigned int findblock(int fd,INODE in,int *block){
  unsigned int inblock[15];

  if(block[0]<11){//direct block
    block[0]++;
    return in.i_block[block[0]];
  }

  if(block[0]==11){ //first time in indirect block
    block[0]++;
    lseek(fd,in.i_block[block[0]]*K,SEEK_SET);
    read(fd,&inblock,15*sizeof(int));
    return inblock[0];
  }
  if(block[0]==12){ //in indirect block
    if(block[1]==14){ //ready to go to first doubly indirect block
      block[1]=0;
      block[2]=-1;
      block[0]++;
    }
    else{
      block[1]++;
      lseek(fd,in.i_block[block[0]]*K,SEEK_SET);
      read(fd,&inblock,15*sizeof(int));
      return inblock[block[1]];
    }
    
  }

  if(block[0]==13){ //doubly indirect block
    if(block[1]==14 && block[2]==14){
      block[0]++;
      block[1]=0;
      block[2]=0;
      printf("Need triply indirect block\n");
    }
    else{
      if(block[2]==15){
	block[2]=0;
	block[1]++;
      }
      else
	block[2]++;
      lseek(fd,K*in.i_block[block[0]],SEEK_SET);
      read(fd,&inblock,15*sizeof(int));
      lseek(fd,K*inblock[block[1]],SEEK_SET);
      read(fd,&inblock,15*sizeof(int));
      return inblock[block[2]];      
    }
  }
  return 0;
  
}
