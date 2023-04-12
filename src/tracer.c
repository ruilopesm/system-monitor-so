#include "tracer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: %s <option>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd;
  do {
    fd = open(MAIN_FIFO_NAME, O_WRONLY);
    if (fd == -1) {
      perror("open");
      sleep(1);
    }
  } while (fd == -1);

  // TODO: Improve and refactor this code
  // (extract the argument parsing logic to another module maybe?)
  char *option = argv[1];

  if (!strcmp(option, "execute")) {
    // FIXME: This only works with a single program,
    // not with programs that have arguments
    char *program = argv[3];  // Because there should be an -u on argv[2]

    int pid = fork();
    if (pid == 0) {
      // Child
      printf("Running PID %d\n", getpid());

      if (execlp(program, program, NULL) == -1) {
        perror("execlp");
        exit(EXIT_FAILURE);
      }
    } else {
      // Parent
      program_info *info = malloc(sizeof(program_info));
      info->pid = pid;
      strcpy(info->name, program);  // NOLINT

      if (write(fd, info, sizeof(program_info)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
      }

      // Close the named pipe
      close(fd);

      exit(EXIT_SUCCESS);
    }
  }

  exit(EXIT_SUCCESS);
}
