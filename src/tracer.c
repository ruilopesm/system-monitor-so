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

int main(int argc, char **argv) {
  if (argc < 2) {
    wprintf("Usage: %s <option>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Wait until the monitor FIFO has been created
  int monitor_fd;
  do {
    monitor_fd = open(MAIN_FIFO_NAME, O_WRONLY);
    if (monitor_fd == -1) {
      perror("open");
      sleep(1);  // Try again within 1 second
    }
  } while (monitor_fd == -1);

  char *option = argv[1];

  if (!strcmp(option, "execute")) {
    char *flag = argv[2];

    if (!strcmp(flag, "-u")) {
      // Single execute
      char *command = strdup(argv[3]);
      char **parsed_command = parse_command(argv[3], NULL, " ");

      if (execute_program(command, parsed_command, monitor_fd) == -1) {
        perror("execute_program");
        exit(EXIT_FAILURE);
      }
    } else {  // flag is -p
      // Pipeline execute
      char *pipeline = strdup(argv[3]);
      int pipeline_cmds_count = 0;
      char **parsed_pipeline =
          parse_command(argv[3], &pipeline_cmds_count, "|");

      if (execute_pipeline(
              pipeline, parsed_pipeline, pipeline_cmds_count, monitor_fd
          ) == -1) {
        perror("execute_pipeline");
        exit(EXIT_FAILURE);
      }
    }
  } else if (!strcmp(option, "status")) {
    if (execute_status(monitor_fd) == -1) {
      perror("execute_status");
      exit(EXIT_FAILURE);
    }
  } else if (!strcmp(option, "stats-time")) {
    int n_pids = argc - 2;
    if (n_pids == 0) {
      wprintf("Usage: %s stats-time <PID-123> <PID-456> ...\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    char **pids = argv + 2;
    int *parsed_pids = parse_pids(pids, n_pids);
    PIDS_ARR *pids_arr = create_pids_arr(parsed_pids, n_pids, getpid());

    if (execute_stats_time(monitor_fd, pids_arr) == -1) {
      perror("execute_stats_time");
      exit(EXIT_FAILURE);
    }
  } else if (!strcmp(option, "stats-command")) {
    int n_pids = argc - 3;
    if (n_pids == 0 || argc == 2) {
      wprintf(
          "Usage: %s stats-command <program> <PID-123> <PID-456> ...\n", argv[0]
      );
      exit(EXIT_FAILURE);
    }

    char *program_name = argv[2];
    char **pids = argv + 3;
    int *parsed_pids = parse_pids(pids, n_pids);
    PIDS_ARR *pids_arr = create_pids_arr(parsed_pids, n_pids, getpid());
    PIDS_ARR_WITH_PROGRAM *pids_arr_with_program =
        create_pids_arr_with_program(*pids_arr, program_name);

    if (execute_stats_command(monitor_fd, pids_arr_with_program) == -1) {
      perror("execute_stats_command");
      exit(EXIT_FAILURE);
    }
  } else if (!strcmp(option, "stats-uniq")) {
    int n_pids = argc - 2;
    if (n_pids == 0) {
      wprintf("Usage: %s stats-uniq <PID-123> <PID-456> ...\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    char **pids = argv + 2;
    int *parsed_pids = parse_pids(pids, n_pids);
    PIDS_ARR *pids_arr = create_pids_arr(parsed_pids, n_pids, getpid());

    if (execute_stats_uniq(monitor_fd, pids_arr) == -1) {
      perror("execute_stats_uniq");
      exit(EXIT_FAILURE);
    }
  } else {
    wprintf("Invalid option\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

int execute_program(char *full_program, char **parsed_program, int monitor_fd) {
  pid_t pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time, final_time;
  gettimeofday(&start_time, NULL);

  pid_t child_pid = fork();
  if (child_pid == 0) {
    // Child
    PROGRAM_INFO *execute_info =
        create_program_info(pid, full_program, start_time);
    write_to_fd(monitor_fd, execute_info, sizeof(PROGRAM_INFO), NEW);

    free(execute_info);

    if (execvp(parsed_program[0], parsed_program) == -1) {
      perror("execvp");
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

      // Ensure server answered with OK
      REQUEST_TYPE *type = malloc(sizeof(REQUEST_TYPE));
      read_from_fd(pid_fd, type);
      if (*type != OK) {
        wprintf("Server answered with an error\n");
        exit(EXIT_FAILURE);
      }

      PROGRAM_INFO *done_info =
          create_program_info(pid, full_program, final_time);
      write_to_fd(monitor_fd, done_info, sizeof(PROGRAM_INFO), UPDATE);

      struct timeval diff;
      timeval_subtract(&diff, &final_time, &start_time);
      wprintf("Ended in %.3lf ms\n", timeval_to_ms(&diff));

      // Clean resources
      free(done_info);
      free(fifo_name);
      free(parsed_program);
      free(full_program);

      exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
  }
}

int execute_status(int monitor_fd) {
  pid_t pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  PROGRAM_INFO *info = create_program_info(pid, "status", start_time);
  write_to_fd(monitor_fd, info, sizeof(PROGRAM_INFO), STATUS);

  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);

  REQUEST_TYPE type;
  REQUEST *answer_data = read_from_fd(pid_fd, &type);

  // Get current timestamp for calculations
  struct timeval current_timestamp;
  gettimeofday(&current_timestamp, NULL);

  struct timeval diff;
  timeval_subtract(&diff, &current_timestamp, &answer_data->initial_timestamp);

  while (type != DONE) {
    printf(
        "Program '%s' running (%d) for %.3lf ms\n", answer_data->command,
        answer_data->pid, timeval_to_ms(&diff)
    );
    answer_data = read_from_fd(pid_fd, &type);
  }

  // Clean resources
  free(info);
  free(answer_data);
  free(fifo_name);

  exit(EXIT_SUCCESS);
}

int execute_pipeline(
    char *pipeline, char **parsed_pipeline, int pipeline_cmds_count,
    int monitor_fd
) {
  pid_t pid = getpid();
  char *fifo_name = create_fifo(pid);

  struct timeval start_time, final_time;
  gettimeofday(&start_time, NULL);

  // Prepare execution
  int original_stdin = dup(STDIN_FILENO);
  int original_stdout = dup(STDOUT_FILENO);

  pid_t child_pid = fork();
  if (child_pid == 0) {
    // Child
    PROGRAM_INFO *info = create_program_info(pid, pipeline, start_time);
    write_to_fd(monitor_fd, info, sizeof(PROGRAM_INFO), PIPELINE);

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
      pid_t pid = fork();
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

        char **parsed_program = parse_command(parsed_pipeline[i], NULL, " ");
        if (execvp(parsed_program[0], parsed_program) == -1) {
          perror("execvp");
          exit(EXIT_FAILURE);
        }
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

      // Get stdin and stdout back to normal
      dup2(original_stdin, STDIN_FILENO);
      dup2(original_stdout, STDOUT_FILENO);
      close(original_stdin);
      close(original_stdout);

      // Ensure server answered with OK
      REQUEST_TYPE *type = malloc(sizeof(REQUEST_TYPE));
      read_from_fd(pid_fd, type);
      if (*type != OK) {
        wprintf("Server answered with an error\n");
        exit(EXIT_FAILURE);
      }

      PROGRAM_INFO *done_info = create_program_info(pid, pipeline, final_time);
      write_to_fd(monitor_fd, done_info, sizeof(PROGRAM_INFO), UPDATE);

      struct timeval diff;
      timeval_subtract(&diff, &final_time, &start_time);
      wprintf("Ended in %.3lf ms\n", timeval_to_ms(&diff));

      // Clean resources
      free(done_info);
      free(fifo_name);

      exit(EXIT_SUCCESS);
    }

    exit(EXIT_FAILURE);
  }
}

int execute_stats_time(int monitor_fd, PIDS_ARR *pids_arr) {
  write_to_fd(monitor_fd, pids_arr, sizeof(PIDS_ARR), STATS_TIME);

  char *fifo_name = create_fifo(pids_arr->child_pid);
  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);
  double *answer_data = read_from_fd(pid_fd, NULL);

  wprintf("Total execution time is %.3lf ms\n", *answer_data);

  exit(EXIT_SUCCESS);
}

int execute_stats_command(
    int monitor_fd, PIDS_ARR_WITH_PROGRAM *pids_arr_with_program
) {
  write_to_fd(
      monitor_fd, pids_arr_with_program, sizeof(PIDS_ARR_WITH_PROGRAM),
      STATS_COMMAND
  );

  char *fifo_name = create_fifo(pids_arr_with_program->pids_arr.child_pid);
  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);
  int *answer_data = read_from_fd(pid_fd, NULL);

  wprintf(
      "%s was executed %d times\n", pids_arr_with_program->program, *answer_data
  );

  // Clean resources
  free(answer_data);
  free(fifo_name);

  exit(EXIT_SUCCESS);
}

int execute_stats_uniq(int monitor_fd, PIDS_ARR *pids_arr) {
  write_to_fd(monitor_fd, pids_arr, sizeof(PIDS_ARR), STATS_UNIQ);

  char *fifo_name = create_fifo(pids_arr->child_pid);
  int pid_fd;
  open_fifo(&pid_fd, fifo_name, O_RDONLY);

  REQUEST_TYPE type;
  char *program_name = read_from_fd(pid_fd, &type);
  while (type != DONE) {
    wprintf("%s\n", program_name);
    free(program_name);
    program_name = read_from_fd(pid_fd, &type);
  }

  // Clean resources
  free(program_name);
  free(fifo_name);

  exit(EXIT_SUCCESS);
}
