#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

// void
// ls(char *path)
// {
//   char buf[512], *p;
//   int fd;
//   struct dirent de;
//   struct stat st;

//   if((fd = open(path, 0)) < 0){
//     fprintf(2, "ls: cannot open %s\n", path);
//     return;
//   }

//   if(fstat(fd, &st) < 0){
//     fprintf(2, "ls: cannot stat %s\n", path);
//     close(fd);
//     return;
//   }

//   switch(st.type){
//   case T_FILE:
//     printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
//     break;

//   case T_DIR:
//     if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
//       printf("ls: path too long\n");
//       break;
//     }
//     strcpy(buf, path);
//     p = buf+strlen(buf);
//     *p++ = '/';
//     while(read(fd, &de, sizeof(de)) == sizeof(de)){
//       if(de.inum == 0)
//         continue;
//       memmove(p, de.name, DIRSIZ);
//       p[DIRSIZ] = 0;
//       if(stat(buf, &st) < 0){
//         printf("ls: cannot stat %s\n", buf);
//         continue;
//       }
//       printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
//     }
//     break;
//   }
//   close(fd);
// }

void
find(char *dir_path, char *file_name){
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(dir_path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", dir_path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", dir_path);
    close(fd);
    return;
  }

  if(st.type != T_DIR){
    fprintf(2, "find: %s is not a dir\n", dir_path);
    close(fd);
    return;
  }

  if(strlen(dir_path) + 1 + DIRSIZ + 1 > sizeof buf){
    fprintf(2, "find: dir_path too long\n");
    return;
  }

  strcpy(buf, dir_path);
  p = buf+strlen(buf);
  *p++ = '/';

  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    //fprintf(2, "find: cat pwd %s\n", buf);
    if(stat(buf, &st) < 0){
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }
    if(st.type == T_DIR){
      find(buf, file_name);
    }
    //fprintf(2, "find: cat file_name and  fmtname %s %s\n",file_name, fmtname(buf));
    //fprintf(2, "find:strcmp %d\n",strcmp(file_name, fmtname(buf)));
    if(0 == strcmp(file_name, fmtname(buf)))
      printf("%s\n", buf);
  }

}

int
main(int argc, char *argv[])
{

  if(argc != 3){
    fprintf(2, "parament number < 3\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
