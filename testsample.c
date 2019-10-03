//#include <stdio.h>

#include "libsample.h"

int main(int argc,char **argv){
  int fd;
  int fd2;
  int rv;
  char c[4096];
  
  //fd=ext2open("/u1/junk/cs471/oct02/disk1/Drinks.txt",O_RDONLY,0);
  /* fd=ext2open("Drinks.txt",O_RDONLY,0);
  //printf("%d\n",fd);
  if(fd<0){
    fprintf(stderr,"Error opening file\n");
    exit(0);
  }
  fd2=ext2open("mysubdir/fibo.py",O_RDONLY,0);*/

  if(argc<2){
    fprintf(stderr,"Usage: %s <filename>\n",argv[0]);
    exit(0);
  }
  fd=ext2open(argv[1],O_RDONLY,0);
  rv=ext2read(fd,c,1025);
  c[rv]=0;
  printf("rv=%d\n",rv);
  printf("%s\n",c);
  ext2seek(fd,99,SEEK_END);
  rv=ext2read(fd,c,99);
  c[rv]=0;
  printf("%s\n",c);
  printf("rv=%d\n",rv);
  ext2close(fd);
  //ext2close(fd2);
}
