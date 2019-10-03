#include <stdio.h>
#include <stdlib.h>

void myfun(){
  printf("%s\n",getenv("HOME"));  
}

int main(){
  myfun();
}
