#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAX_LINE 1024

char *params[MAXARG];
char line[MAX_LINE];

int
main(int argc, char* argv[]) {
  int i, arg_index = 0, pid, n, add_index;
  char *p, *from;
  if (argc < 2) {
    fprintf(2, "usage: xargs cmd\n");
    exit(1);
  }
  for(i = 1; i < argc; i++) {
    params[arg_index++] = argv[i];
  }
  if (arg_index >= MAX_LINE) {
    fprintf(2, "xargs: too many args\n");
    exit(1);
  }
  for(;;) {
    add_index = arg_index;
    p = line;
    from = line;
    while(p != line + MAX_LINE && (n = read(0, p, 1) == 1)) {
      if(*p == ' ') {
        *p = '\0';
        if (add_index >= MAXARG) {
          fprintf(2, "xargs: too many args\n");
          exit(1);
        }
        params[add_index++] = from;
        from = p + 1;
      } else if(*p == '\n') {
        *p = '\0';
        if (add_index >= MAXARG) {
          fprintf(2, "xargs: too many args\n");
          exit(1);
        }
        params[add_index++] = from;
        break;
      }
      p++;
    }
    if (arg_index >= MAXARG) {
      fprintf(2, "xargs: too many args\n");
      exit(1);
    }
    params[add_index] = 0;
    pid = fork();
    if(pid == 0) {
      exec(argv[1], params);
    } else {
      wait(0);
    }
    // read all chars
    if(n == 0) {
      break;
    }
  }
  exit(0);
}
