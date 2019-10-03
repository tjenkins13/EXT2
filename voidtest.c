#include <stdio.h>

int main(){
  char num =0;

  void *ptr=&num;

  printf("%d\n",sizeof(&ptr));
}
