#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void
handle_primes(int left_fd) {
  uint32 base;
  if (read(left_fd, (void*)&base, 4) == 4) {
    printf("prime %d\n", base);
  } else {
    exit(0);
  }
  int write_fd[2];
  pipe(write_fd);
  int pid;
  int i;
  pid = fork();
  if(pid == 0) {
    close(write_fd[1]);
    handle_primes(write_fd[0]);
  } else if(pid > 0) {
    close(write_fd[0]);
    while((read(left_fd, (void*)&i, 4) == 4)) {
      if (i % base != 0) {
        write(write_fd[1], (void*)&i, 4);
      }
    }
    close(left_fd);
    close(write_fd[1]);
    wait(0);
    exit(0);
  } else {
    fprintf(2, "primes: fork error\n");
    exit(1);
  }

}

int
main(int argc, char *argv[]) {
  uint32 i;
  int pid;
  int fd[2];
  pipe(fd);
  pid = fork();
  if (pid == 0) {
    close(fd[1]);
    handle_primes(fd[0]);
  } else if (pid > 0) {
    close(fd[0]);
    for(i = 2; i <= 35; i++) {
      write(fd[1], (void*)&i, 4);
    }
    close(fd[1]);
    wait(0);
  } else {
    fprintf(2, "primes: fork error\n");
    exit(1);
  }
  exit(0);
}
