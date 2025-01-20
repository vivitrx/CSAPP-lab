#include<stdio.h>
int main(int argc, char** argv)
{
  printf("argv[%d] \n", argc);
  for (int i = 0; i < argc; i++)
  {
    printf("argv[%s] \n", argv[i]);
  }
  
    char buf[33];
    printf("\n");
  return 9;
}