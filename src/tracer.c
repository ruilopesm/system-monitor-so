#include "tracer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "parser.h"
#include "utils.h"

// Forward declarations
int execute_program(char *program_name, char **program, int monitor_fd);
int execute_status(int fd);
// End forward declarations

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
    char **program = parse_command(argv_copy);
    char *program_name = program[0];

    if (execute_program(program_name, program, fd) == -1) {
      perror("execute_program");
      exit(EXIT_FAILURE);
    }
  } else if (!strcmp(option, "status")) {
    if (execute_status(fd) == -1) {
      perror("execute_status");
      exit(EXIT_FAILURE);
    }
  }

  exit(EXIT_SUCCESS);
}

int execute_program(char *program_name, char **program, int monitor_fd) {
  int pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  struct timeval final_time;

  int child_pid = fork();
  if (child_pid == 0) {
    // Child
    PROGRAM_INFO *execute_info =
        create_program_info(pid, program[0], start_time.tv_usec);

    if (write_to_fd(monitor_fd, execute_info, sizeof(PROGRAM_INFO), NEW) ==
        -1) {
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
      // Child finished
      gettimeofday(&final_time, NULL);
      PROGRAM_INFO *done_info =
          create_program_info(pid, program[0], final_time.tv_usec);

      // Ensure server answered with OK
      PROGRAM_INFO *answer_info = malloc(sizeof(PROGRAM_INFO));
      read_from_fd(pid_fd, answer_info, sizeof(PROGRAM_INFO));

      write_to_fd(monitor_fd, done_info, sizeof(PROGRAM_INFO), UPDATE);

      struct timeval diff;
      timeval_subtract(&diff, &final_time, &start_time);
      printf("Ended in %ld ms\n", diff.tv_usec / 1000 + diff.tv_sec * 1000);

      // Close the named pipe
      close(monitor_fd);
      free(done_info);
      free(fifo_name);

      exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
  }
}

int execute_status(int fd) {
  int pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  PROGRAM_INFO *info = create_program_info(pid, "status", start_time.tv_usec);

  write_to_fd(fd, info, sizeof(PROGRAM_INFO), STATUS);

  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);

  REQUEST *answer_data = malloc(sizeof(REQUEST));
  // Read data from the named pipe
  while (read_from_fd(pid_fd, answer_data, sizeof(REQUEST)) > 0) {
    printf("pid: %d\n", answer_data->pid);
  }

  free(info);
  free(answer_data);
  free(fifo_name);
  close(pid_fd);

  exit(EXIT_SUCCESS);
}
