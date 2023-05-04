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
#include "requests.h"
#include "utils.h"

// Forward declarations
int execute_program(char *program_name, char **program, int monitor_fd);
int execute_status(int fd);
int execute_pipeline(char *pipeline, int fd);
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
    char *flag = argv[2];

    if (!strcmp(flag, "-u")) {
      // Single execute
      char *argv_copy = strdup(argv[3]);
      char **program = parse_command(argv_copy);
      char *program_name = program[0];

      if (execute_program(program_name, program, fd) == -1) {
        perror("execute_program");
        exit(EXIT_FAILURE);
      }
    } else {  // flag is -p
      // Pipeline execute
      char *pipeline = strdup(argv[3]);

      if (execute_pipeline(pipeline, fd) == -1) {
        perror("execute_pipeline");
        exit(EXIT_FAILURE);
      }
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

  struct timeval start_time, final_time;
  gettimeofday(&start_time, NULL);

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

      // Ensure server answered with OK
      REQUEST_TYPE response = read_from_fd(pid_fd, NULL, sizeof(HEADER));
      if (response != OK) {
        printf("Server answered with an error\n");
        exit(EXIT_FAILURE);
      }

      gettimeofday(&final_time, NULL);
      PROGRAM_INFO *done_info =
          create_program_info(pid, program[0], final_time.tv_usec);
      write_to_fd(monitor_fd, done_info, sizeof(PROGRAM_INFO), UPDATE);

      struct timeval diff;
      timeval_subtract(&diff, &final_time, &start_time);
      printf("Ended in %ld ms\n", diff.tv_usec / 1000 + diff.tv_sec * 1000);

      // Close the named pipe
      free(done_info);
      free(fifo_name);
      free(program_name);
      free(program);

      exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
  }
}

int execute_status(int fd) {
  int pid = getpid();
  printf("PID: %d\n", pid);
  char *fifo_name = create_fifo(pid);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  PROGRAM_INFO *info = create_program_info(pid, "status", start_time.tv_usec);
  write_to_fd(fd, info, sizeof(PROGRAM_INFO), STATUS);

  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);

  REQUEST *answer_data = malloc(sizeof(REQUEST));
  while (read_from_fd(pid_fd, answer_data, sizeof(REQUEST)) != DONE) {
    printf("Program %s running (%d)\n", answer_data->command, answer_data->pid);
  }

  free(info);
  free(answer_data);
  free(fifo_name);

  exit(EXIT_SUCCESS);
}

int execute_pipeline(char *pipeline, int fd) {
  int pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time, final_time;
  gettimeofday(&start_time, NULL);

  PROGRAM_INFO *info = create_program_info(pid, pipeline, start_time.tv_usec);
  write_to_fd(fd, info, sizeof(PROGRAM_INFO), PIPELINE);

  char *pipeline_cmds[2];
  int pipeline_cmds_count = parse_pipeline(pipeline, pipeline_cmds);

  int origin_stdin = dup(STDIN_FILENO);
  int origin_stdout = dup(STDOUT_FILENO);

  int *pipes = malloc(sizeof(int) * pipeline_cmds_count * 2);
  for (int i = 0; i < pipeline_cmds_count; i++) {
    if (pipe(&pipes[i * 2]) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }

  int *child_pids = malloc(sizeof(int) * pipeline_cmds_count);
  for (int i = 0; i < pipeline_cmds_count; i++) {
    child_pids[i] = -1;
  }

  for (int i = 0; i < pipeline_cmds_count; i++) {
    int pid = fork();
    if (pid == 0) {
      // Child
      if (i == 0) {
        // First command
        close(pipes[0]);
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[1]);
      } else if (i == pipeline_cmds_count - 1) {
        // Last command
        close(pipes[(i - 1) * 2 + 1]);
        dup2(pipes[(i - 1) * 2], STDIN_FILENO);
        close(pipes[(i - 1) * 2]);
      } else {
        // Middle command
        close(pipes[(i - 1) * 2 + 1]);
        dup2(pipes[(i - 1) * 2], STDIN_FILENO);
        close(pipes[(i - 1) * 2]);

        close(pipes[i * 2]);
        dup2(pipes[i * 2 + 1], STDOUT_FILENO);
        close(pipes[i * 2 + 1]);
      }

      // Close all pipe file descriptors in the child process
      for (int j = 0; j < pipeline_cmds_count; j++) {
        close(pipes[j * 2]);
        close(pipes[j * 2 + 1]);
      }

      char *copy = strdup(pipeline_cmds[i]);
      char **program = parse_command(copy);
      char *program_name = program[0];

      if (execvp(program_name, program) == -1) {
        perror("execvp");
        exit(EXIT_FAILURE);
      }
    } else {
      // Parent
      child_pids[i] = pid;
    }
  }

  // Close all pipes
  for (int i = 0; i < pipeline_cmds_count; i++) {
    close(pipes[i * 2]);
    close(pipes[i * 2 + 1]);
  }

  // Wait for all childs
  for (int j = 0; j < pipeline_cmds_count; j++) {
    int status;
    if (waitpid(child_pids[j], &status, 0) == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
  }

  // Get stdin and stdout back to normal
  dup2(origin_stdin, STDIN_FILENO);
  dup2(origin_stdout, STDOUT_FILENO);
  close(origin_stdin);
  close(origin_stdout);

  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);

  // Ensure server answered with OK
  REQUEST_TYPE response = read_from_fd(pid_fd, NULL, sizeof(HEADER));
  if (response != OK) {
    printf("Server answered with an error\n");
    exit(EXIT_FAILURE);
  }

  gettimeofday(&final_time, NULL);
  PROGRAM_INFO *done_info =
      create_program_info(pid, pipeline, final_time.tv_usec);
  write_to_fd(fd, info, sizeof(PROGRAM_INFO), UPDATE);

  struct timeval diff;
  timeval_subtract(&diff, &final_time, &start_time);
  printf("Ended in %ld ms\n", diff.tv_usec / 1000 + diff.tv_sec * 1000);

  // Close the named pipe
  free(info);
  free(fifo_name);
  free(done_info);
  free(pipes);
  free(child_pids);

  exit(EXIT_SUCCESS);
}
