#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
  char *s="hello";
  char c='a';
  //  memcpy(s, &c, 1);
  printf("%c\n",*(s+1));
  *s++;
  printf("%c\n",*s);
}
