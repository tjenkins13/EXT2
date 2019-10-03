#ifndef EXT2_DEF
#define EXT2_DEF
#include "libsample2.h"
#endif


int fdinitflag=1;

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

  printf("%s %2d %2d:%02d",months[tp->tm_mon],tp->tm_mday,tp->tm_hour,tp->tm_min);

}

unsigned int findblock(int fd,INODE in,int *block){
  unsigned int inblock[K/4];

  if(block[0]<=11){//direct block
    block[0]++;
    return in.i_block[block[0]-1];
  }

  /*if(block[0]==11){ //first time in indirect block
    block[0]++;
    lseek(fd,in.i_block[block[0]]*K,SEEK_SET);
    read(fd,&inblock,K/4*sizeof(int));
    return inblock[0];
    }*/
  if(block[0]==12){ //in indirect block
    if(block[1]==((K/4)-1)){ //ready to go to first doubly indirect block
      block[1]=-1;
      block[2]=-1;
      block[0]++;
    }
    else{
      block[1]++;
      lseek(fd,in.i_block[block[0]]*K,SEEK_SET);
      read(fd,&inblock,K/4*sizeof(int));
      return inblock[block[1]-1];
    }

  }

  if(block[0]==13){ //doubly indirect block
    if(block[1]==((K/4)-1) && block[2]==((K/4)-1)){
      block[0]++;
      block[1]=-1;
      block[2]=-1;
      block[3]=-1;
      printf("Need triply indirect block\n");
    }
    else{
      if(block[2]==((K/4)-1)){
        block[2]=0;
        block[1]++;
      }
      else
        block[2]++;
      lseek(fd,K*in.i_block[block[0]],SEEK_SET);
      read(fd,&inblock,K/4*sizeof(int));
      lseek(fd,K*inblock[block[1]],SEEK_SET);
      read(fd,&inblock,K/4*sizeof(int));
      return inblock[block[2]];
    }
  }
  if(block[0]==14){
    if(block[1]==((K/4)-1) && block[2]==((K/4)-1) && block[3]==((K/4)-1)){
      printf("Too much memory used\n");
      return 0;
    }
    if(block[3]==((K/4)-1)){
      block[3]=0;
      if(block[2]==((K/4)-1)){
        block[2]=0;
        if(block[1]==((K/4)-1)){
          printf("Too much memory used\n");
          return 0;
        }
        else
          block[1]++;
      }
      else
        block[2]++;
    }
    else
      block[3]++;
    lseek(fd,K*in.i_block[block[0]],SEEK_SET);
    read(fd,&inblock,K/4*sizeof(int));
    lseek(fd,K*inblock[block[1]],SEEK_SET);
    read(fd,&inblock,K/4*sizeof(int));
    lseek(fd,K*inblock[block[2]],SEEK_SET);
    read(fd,&inblock,K/4*sizeof(int));
    return inblock[block[3]];
      }
  return 0;

}

void myls(int fd,DIR_STRUCT dir,INODE in){ //do ls for single file
  printf("%2d ",dir.inode);
  printmode(in);
  printf(" %2d %5s %5s %5d ",in.i_links_count,
         getpwuid((uid_t)in.i_uid)->pw_name,
         getgrgid((gid_t)in.i_gid)->gr_name,
         in.i_size);

  printdate(in);
  if(in.i_mode & EXT2_S_IFDIR)
    printf(ANSI_COLOR_BLUE " %s" ANSI_COLOR_RESET "/\n",dir.name); //https://stackoverflow.com/questions/3219393/stdlib-and-colored-output-in-c                                               
  else{
    printf(" %s\n",dir.name);
  }

}

void print_ino(int fd,INODE in,int block_size){
  int pos=0;
  int blocks[4]={0,0,0,0};
  int block=findblock(fd,in,blocks);
  unsigned char c=1;
  int bread=0;
  lseek(fd,block * block_size,SEEK_SET);
  while(bread<=in.i_size && c){
    read(fd,&c,1);
    pos++;
    //bread++;
    printf("%c",c);
    if(pos>=block_size){
      block=findblock(fd,in,blocks);
      lseek(fd,block*block_size,SEEK_SET);
      pos=0;
    }
  }

}



void myls2(int fd,INODE root,int block_size,SUPERBLOCK sb,BGDT bg){ //do ls for all files in directory
  DIR_STRUCT dir;
  int pos;
  int block;
  int blocks[4]={0,0,0,0};
  int rv=0;
  INODE in;

  dir.inode=1;
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
  }
  return;
}

unsigned int search(int fd,INODE root,int block_size,SUPERBLOCK sb,BGDT bg,char *fname){//search for file with name fname in directory root
  DIR_STRUCT dir;
  int pos;
  int block;
  int blocks[4]={0,0,0,0};
  int rv=0;
  INODE in;

  dir.inode=1;
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
      //myls(fd,dir,in);
      if(strncmp(fname,dir.name,strlen(fname))==0)
        return dir.inode-1;
    }
    if(pos>=block_size){
      pos=0;
      block=findblock(fd,root,blocks);
    }
  }
  return 0;
}


int split(char *fname,char *pieces){ //split path into pieces
  int i=0;
  char a[K];

  for(i=1;(i<strlen(fname)) && fname[i]!='/';i++);

  strncpy(pieces,fname,i);
  pieces[i]=0;
  //fname+=i;
  if(fname[i]=='/')
    return i+1;
  return -1;
  printf("%s %s\n",fname,pieces);

  printf("%d\n",i);
}

/*int findfile(int fd,const char *fname,SUPERBLOCK sb,BGDT bg,int block_size,INODE root){
  char *namecp=strdup(fname);
  char pieces[K];
  int disp,tdisp=0;
  DIR_STRUCT dir;
  INODE in=root;
  int blocks[4];
  int block;
  int pos;
  
  while((in.i_mode & EXT2_S_IFDIR) && namecp ){
    disp=split(namecp,pieces);
    printf("%s\n",pieces);
    if(disp<0)//didn't find '/'
      break;
    tdisp+=disp;
    namecp+=disp;
    blocks[0]=blocks[1]=blocks[2]=blocks[3]=0;
    block=0;
    block=findblock(fd,in,blocks);
    
  }
  printf("%s\n",pieces);
  namecp-=tdisp;
  free(namecp);
  }*/


int myopen(int fd, char *fname,INODE root,int block_size,SUPERBLOCK sb,BGDT bg){//find file within already opened filesystem
  DIR_STRUCT dir;
  INODE in=root;
  //  INODE f;
  int disp,rv;
  char pieces[K];
  unsigned int inloc=0;
  char fcpy[K];
  strcpy(fcpy,fname);//copy filename
  //  f.i_mode=f.i_size=0;

    if(!(root.i_mode & EXT2_S_IFDIR)){//directory given not directory
      fprintf(stderr,"Inode given not directory\n");
      return -1;
    }

    while(fname && pieces){
      disp=split(fname,pieces); //split pathname up into direcory name and remaining path
      if(disp<0) //no more directories to go to
        break;
      fname+=disp;
      inloc=search(fd,in,block_size,sb,bg,pieces); //find index of inode for directory
      //printf("%d %s %s\n",inloc,pieces,fname);
      if(inloc==0){
        fprintf(stderr,"File %s not found\n",fcpy);
        exit(0);
      }
      lseek(fd,(bg.bg_inode_table*block_size)+(inloc * sb.s_inode_size),SEEK_SET); //go to where directory is
      rv=read(fd,&in,sizeof(INODE)); //read new directory
      if(rv!=sizeof(INODE)){
        fprintf(stderr,"Bad inode read\n");
        exit(0);
      }
      if(!(in.i_mode & EXT2_S_IFDIR)){
        fprintf(stderr,"%s is not a directory\n",pieces);
        exit(0);
      }
    }
    inloc=search(fd,in,block_size,sb,bg,pieces);//find location of inode of file
    // printf("%s\n",pieces);
    if(inloc==0){
      fprintf(stderr,"File \'%s\' not found\n",fcpy);
      exit(0);
    }
    lseek(fd,(bg.bg_inode_table*block_size)+(inloc * sb.s_inode_size),SEEK_SET);//go to where file's inode is
    rv=read(fd,&in,sizeof(INODE));
    if(rv!=sizeof(INODE)){
      fprintf(stderr,"Bad inode read\n");
      exit(0);
    }
    if(in.i_mode & EXT2_S_IFDIR) //just do ls -- will return -1
      myls2(fd,in,block_size,sb,bg);
    else if(in.i_mode & EXT2_S_IFREG){ //print file
      //print_ino(fd,in,block_size);
      return inloc;
        }
    //printf("%s\n",pieces);
  
  
  return -1;
}


int ext2open(const char *pathname, int flags, int mode){ //open file descriptor for file in ext2 filesystem
  int fd=1,block_size;
  int ino_index=-1;
  int i,rv;
  int tdisp=0;
  int disp;
  SUPERBLOCK sb;
  BGDT bg;
  INODE root,in;
  char *fname=strdup(pathname); //get copy of pathname
  char path[K];
  char pieces[K];

  if(fdinitflag){ //initialize fd array if uninitialized
  for(i=0;i<FD_NUM;i++){
    fdar[i].fd=-1;
  }
  fdinitflag=0;
  }


  if(!getenv("DISK_NAME")){ //treat pathname as <path_to_disk>/<path_in_disk> if variable undefined
      while(fd>0){ //break pathname into path to disk and path in disk
        disp=split(fname,pieces);
        if(disp<0)//didn't find '/'
          break;
        tdisp+=disp;
        fname+=disp;
  //printf("%s\n",fname);
        if(!path[0]){
          strcpy(path,pieces);
        }
        else
          sprintf(path,"%s/%s",path,pieces);
        fd=open(path,O_DIRECTORY);
        close(fd);
      }
      sprintf(pieces,"%s",fname);
      if(fd>0 && fname){//stopped at directory, not file - go another step
        sprintf(path,"%s/%s",path,fname);
        pieces[0]='.';
        pieces[1]=0;
      }

      //printf("%s\n%s\n",path,pieces);
      fname-=tdisp;
      free(fname); //free memory to copy of fname
    }
  else{ //use environmental variable
      sprintf(path,"%s",getenv("DISK_NAME"));
      sprintf(pieces,"%s",fname);
    }
    
  if(flags == O_RDONLY){//if mode is readonly
    fd=open(path,O_RDONLY); //open disk
    if(fd<1){
      fprintf(stderr,"File %s not found\n",pathname);
      return -1;
    }
  
    lseek(fd,K,SEEK_SET); //go to superblock
    rv=read(fd,&sb,sizeof(SUPERBLOCK));//read superblock
    if(rv!=sizeof(SUPERBLOCK)){
      fprintf(stderr,"Bad superblock read\n");
      close(fd);
      return -1;;
    }
    block_size=K<<sb.s_log_block_size; //get block size
    if(block_size>K){                  //if block size not K go to where block group descriptor table is
      lseek(fd,block_size,SEEK_SET);
    }
    rv=read(fd,&bg,sizeof(BGDT));     //read block group descriptor table
    if(rv!=sizeof(BGDT)){
      fprintf(stderr,"Bad block group descriptor table read\n");
      close(fd);
      return -1;
    }
    lseek(fd,(bg.bg_inode_table*block_size)+(sb.s_inode_size),SEEK_SET); //go to where root inode is
    rv=read(fd,&root,sizeof(INODE)); //read root inode
    if(rv!=sizeof(INODE)){
      fprintf(stderr,"Bad inode read\n");
      close(fd);
      return -1;
    }
    //need to break up path in disk and go to each inode until at file
    //findfile(fd,pieces,sb,bg,block_size,root);
    ino_index=myopen(fd,pieces,root,block_size,sb,bg); //find index in inode table for inode of file
    if(ino_index<0){
      close(fd);
      fprintf(stderr,"File %s not found\n",pathname);
      return -1;
    }

    for(i=0;fdar[i].fd>=0;i++); //find index of usable file descriptor in array

    fdar[i].fd=fd;
    fdar[i].blocks[0]=fdar[i].blocks[1]=fdar[i].blocks[2]=fdar[i].blocks[3]=0;
    fdar[i].block=0;
    fdar[i].block_size=block_size;
    fdar[i].ino_size=sb.s_inode_size;

    lseek(fd,(bg.bg_inode_table*block_size)+(sb.s_inode_size*ino_index),SEEK_SET); //go to where file's inode is
    rv=read(fd,&in,sizeof(INODE));
    if(rv!=sizeof(INODE)){//should never happen--already found index of inode successfully
      fprintf(stderr,"Bad inode read\n");
      close(fd);
      return -1;
    }
    fdar[i].in=in;
    
    fdar[i].ino_index=ino_index;
    fdar[i].sb=sb;
    fdar[i].bg=bg;
    fdar[i].offset=0;    
    return i;
  }
  else //for if user wants to do something other than just read -- to be added
    return -1;
}

int ext2seek(int fd, int offset, int whence){
  int pos=0;
  char c=1;
  int rv;

  if(fd<0)
    return -1;
  if(fdar[fd].fd<0)
    return -1;
  
  if(whence==SEEK_SET){ //start from beginning
    fdar[fd].offset=offset;
  }
  else if(whence==SEEK_CUR){ //start from current offset
    fdar[fd].offset+=offset;
  }
  else if(whence ==SEEK_END){ //go from end
    fdar[fd].offset=fdar[fd].in.i_size-offset;
    if(fdar[fd].offset<0) //tried going farther than file size from end
      fdar[fd].offset=0;
  }
  else{
    fprintf(stderr,"invalid whence for lseek\n");
    return -1;
  }

  if(fdar[fd].offset>fdar[fd].in.i_size){
    fprintf(stderr,"Tried seeking too far in file\n");
    return -1;
  }
  fdar[fd].blocks[0]=fdar[fd].blocks[1]=fdar[fd].blocks[2]=fdar[fd].blocks[3]=0;
  fdar[fd].block=findblock(fdar[fd].fd,fdar[fd].in,fdar[fd].blocks); //find first block
  lseek(fdar[fd].fd,fdar[fd].block_size*fdar[fd].block,SEEK_SET); //go to first block
  
  while(pos<fdar[fd].offset && pos<fdar[fd].in.i_size && c!=EOF){ //read until to new offset
    rv=read(fdar[fd].fd,&c,sizeof(char));
    if(rv!=sizeof(char))
      break;
    pos++;
    if(!(pos%fdar[fd].block_size) && pos){ //need to go to new block
      fdar[fd].block=findblock(fdar[fd].fd,fdar[fd].in,fdar[fd].blocks);
      lseek(fdar[fd].fd,fdar[fd].block_size*fdar[fd].block,SEEK_SET);
    }
  }
  return pos; //how far seeked from beginning of file
}

int ext2read(int fd, void *buf, int count){
  int nread=0;          //number of read bytes
  unsigned char c=1;    //char to read file byte by byte
  int rv;               //return value of read
  unsigned char *s;     //for storing value of read bytes

  if(fd<0){
    fprintf(stderr,"Invalid file descriptor\n");
    return -1;
  }
  if(count < 1){
    fprintf(stderr,"Invalid number of bytes given");
    return -1;
  }
  if(fdar[fd].fd<0){
    fprintf(stderr,"Invalid file descriptor\n");
    return -1;    
  }
  
  s=(unsigned char *)malloc(sizeof(char)*count); //read at most count bytes
  if(s==NULL){
    fprintf(stderr,"Error mallocing\n");
    exit(0);
  }
  //s[0]=0;
  //print_ino(fdar[fd].fd,fdar[fd].in,fdar[fd].block_size);
  
  if(fdar[fd].block==0){ //first time reading from file descriptor
    fdar[fd].block=findblock(fdar[fd].fd,fdar[fd].in,fdar[fd].blocks);
    lseek(fdar[fd].fd,fdar[fd].block_size*fdar[fd].block,SEEK_SET);          
  }
  
  while(nread<count && c!=EOF && fdar[fd].offset<fdar[fd].in.i_size){ //read count bytes or till end of file
    rv=read(fdar[fd].fd,&c,sizeof(unsigned char));
    if(rv!=sizeof(unsigned char))
      break;
    //printf("%c",c);
    s[nread]=c;
    nread++;
    fdar[fd].offset++;
    if(!(fdar[fd].offset%fdar[fd].block_size) && fdar[fd].offset){ //need another block 
      fdar[fd].block=findblock(fdar[fd].fd,fdar[fd].in,fdar[fd].blocks);
      lseek(fdar[fd].fd,fdar[fd].block_size*fdar[fd].block,SEEK_SET);      
    }
  }
  //s[nread]=0;
  //printf("%s\n",s);
  memcpy(buf, s,nread); //copy stored array to buf
  free(s);
  //buf=(void *)s;
  return nread; //return number of bytes read
}

int ext2close(int op){ //close open ext2 file system
  if(op<0)
    return -1;
  if(fdar[op].fd<0)
    return -1;
  close(fdar[op].fd);
  fdar[op].fd=-1;
  fdar[op].blocks[0]=fdar[op].blocks[1]=fdar[op].blocks[2]=fdar[op].blocks[3]=0;
  fdar[op].block=0;
  fdar[op].block_size=0;
  fdar[op].ino_size=0;
  fdar[op].ino_index=0;
  fdar[op].offset=0;
  return 0;
}
