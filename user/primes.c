#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void recur(int size, int nums[]){
  int p[2];
  pipe(p);
  int pid = fork();
  if(pid == 0){
    close(p[1]);
    char byte;
    size = 0;
    while(read(p[0],&byte ,1) != 0){
      nums[size ++] = byte;
    }
    close(p[0]);

    if(size == 0)
      exit(0);
    recur(size, nums);
  }else{
    //fprintf(2, "new process %d\n", getpid());
    fprintf(1, "prime %d\n", nums[0]);
    close(p[0]);
    char byte;
    for(int i = 1; i < size; i ++){
      if(nums[i] % nums[0] != 0){
        byte = nums[i];
        write(p[1],&byte ,1);
      }
    }
    close(p[1]);
    wait(0);
  }
}
int
main(int argc, char *argv[])
{
  int size = 34;
  int nums[34] = {0};
  for(int i = 0; i < 34; i ++)
    nums[i] = i + 2;
  recur(size, nums);
  exit(0);
}
