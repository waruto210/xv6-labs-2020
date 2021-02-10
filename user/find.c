#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char *path, char *file) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type) {
  case T_FILE:
    fprintf(2, "%s is not a dir\n", path);
    close(fd);
    return;
  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
      if(strcmp(de.name, ".") && strcmp(de.name, "..")) {
        if (de.inum == 0)
          continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = '\0';
        if(stat(buf, &st) < 0){
          printf("find: cannot stat %s\n", buf);
          continue;
        }
        switch(st.type) {
        case T_FILE:
          if (!strcmp(p, file)) {
            printf("%s\n", buf);
          }
          break;
        case T_DIR:
          find(buf, file);
          break;
        }
      }
    }
    break;
  }
  close(fd);

}

int
main(int argc, char *argv[]){
  if(argc != 3) {
    printf("usage: find path file\n");
  }
  find(argv[1], argv[2]);
  exit(0);
}
