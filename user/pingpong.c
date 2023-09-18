#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2];
  pipe(p1);

  int pid = fork();
  if(pid == 0){
    char byte = 'a';
    read(p1[0], &byte, 1);
    fprintf(2, "%c: received ping\n", byte);
    fprintf(1, "%d: received ping\n", getpid());
    byte = 'f';
    write(p1[1], &byte, 1);
    close(p1[0]);
    close(p1[1]);
    exit(0);
  }else{
    char byte = 'b';
    write(p1[1], &byte, 1);
    wait(0);

    read(p1[0], &byte, 1);
    fprintf(2, "%c: received pong\n", byte);
    fprintf(1, "%d: received pong\n", getpid());
    close(p1[0]);
    close(p1[1]);
    exit(0);
  }
}
