#include "tracer.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"
#include "utils.h"

int execute_program(char *program_name, char **program, int monitor_fd) {
  int pid = getpid();
  
  char *fifo_name = create_fifo(pid);
  
  int child_pid = fork();
  if (child_pid == 0) {
    // Child
    program_info *execute_info =
        create_program_info(pid, program[0], NEW);

    if (write(monitor_fd, execute_info, sizeof(program_info)) == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }
    free(execute_info);

    if (execvp(program_name, program) == -1) {
      perror("execlp");
      exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
  } else {
    // Parent

    // Wait for child to finish
    int pid_fd;
    open_fifo(&pid_fd, fifo_name, O_RDONLY);

    int status;
    if (wait(&status) > 0 && WIFEXITED(status)) {
      program_info *done_info = create_program_info(pid, program[0], UPDATE);

      // Read data from the named pipe
      program_info *answer_info = malloc(sizeof(program_info));
      int read_bytes = read_from_pipe(pid_fd, answer_info);

      if (answer_info->type == ERROR) {
        perror("server error");
        exit(EXIT_FAILURE);
      }

      // TODO: change the fd to the pid_fd
      write_to_pipe(monitor_fd, done_info);

      // Close the named pipe
      close(monitor_fd);
      free(done_info);
      free(answer_info);
      free(fifo_name);

      exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
  }
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

  char *option = argv[1];

  if (!strcmp(option, "execute")) {
    char *argv_copy = strdup(argv[3]);
    char **program =
        parse_command(argv_copy);  // Because there should be an -u on argv[2]
    char *program_name = program[0];

    if (execute_program(program_name, program, fd) == -1) {
      perror("execute_program");
      exit(EXIT_FAILURE);
    }
  }

  exit(EXIT_SUCCESS);
}
