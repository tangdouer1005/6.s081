#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    fprintf(2, "Sleep: argument number wrong\n");
    exit(1);
  }
	char *pos = argv[1];
	while(pos){
		if(*pos < '0' || *pos > '9'){
			fprintf(2, "Sleep: %s is not a int\n", argv[1]);
			exit(1);
		}
	}
  int n = atoi(argv[1]);
	if(sleep(n) == 0)
		exit(0);
	else
		exit(1);
}
