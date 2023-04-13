#include "tracer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "utils.h"

program_info *create_program_info(int pid, char *name, enum request_type type) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  program_info *info = malloc(sizeof(program_info));

  info->pid = pid;
  strcpy(info->name, name);  // NOLINT
  info->timestamp = tv.tv_usec;
  info->type = type;

  return info;
}

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
      program_info *execute_info = create_program_info(getpid(), program, EXECUTE);

      if (write(fd, execute_info, sizeof(program_info)) == -1) {
        perror("write");
        exit(EXIT_FAILURE);
      }
      free(execute_info);

      if (execlp(program, program, NULL) == -1) {
        perror("execlp");
        exit(EXIT_FAILURE);
      }

      exit(EXIT_SUCCESS);
    } else {
      // Parent

      // wait for child to finish
      int status;
      if (( pid = wait(&status) ) > 0 && WIFEXITED(status)) {
          program_info *done_info = create_program_info(pid, program, DONE);

          if (write(fd, done_info, sizeof(program_info)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
          }

          // Close the named pipe
          close(fd);
          free(done_info);

          exit(EXIT_SUCCESS);
      }

      exit(EXIT_FAILURE);
    }
  }

  exit(EXIT_SUCCESS);
}
