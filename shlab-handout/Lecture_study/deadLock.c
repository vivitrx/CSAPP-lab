#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
// deadLock
void deadLock_handler(int sig){
  printf("deadlock");
  exit(0);
}
int main(){
  if (signal(SIGINT, deadLock_handler) == SIG_ERR) {
    perror("Error setting signal handler");
    return 1;
  }
  while (1)
  {
    printf("in main printf\n");
  }
  return 1;
}