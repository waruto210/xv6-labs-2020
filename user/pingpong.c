#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[]) {
  int pid;
  int child_read[2];
  int parent_read[2];
  char buf = '0';
  pipe(child_read);
  pipe(parent_read);
  pid = fork();
  // child
  if (pid == 0) {
    close(child_read[1]);
    close(parent_read[0]);
    pid = getpid();
    if ((read(child_read[0], (void*)&buf, 1)) == 1) {
      printf("%d: received ping\n", pid);
    } else {
      fprintf(1, "pingpong: child received error\n");
      exit(1);
    }
    close(child_read[0]);
    write(parent_read[1], (void*)&buf, 1);
    close(parent_read[1]);
    exit(0);
  } else if(pid > 0) {
    // parent
    close(child_read[0]);
    close(parent_read[1]);
    pid = getpid();
    write(child_read[1], (void*)&buf, 1);
    close(child_read[1]);
    if ((read(parent_read[0], (void*)&buf, 1)) == 1) {
      printf("%d: received pong\n", pid);
    } else {
      fprintf(1, "pingpong: parent received error\n");
      exit(1);
    }
    close(parent_read[0]);
    wait(0);
    exit(0);
  } else {
    fprintf(1, "pingpong: fork error\n");
    exit(1);
  }
}
