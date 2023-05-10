#include "requests.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "monitor.h"
#include "utils.h"

REQUESTS_ARRAY *create_requests_array(size_t size) {
  REQUESTS_ARRAY *requests_array = malloc(sizeof(REQUESTS_ARRAY));

  requests_array->requests = malloc(sizeof(REQUEST *) * size);
  requests_array->current_index = 0;
  requests_array->capacity = size;

  return requests_array;
}

REQUEST *create_request(
    pid_t pid, struct timeval initial_timestamp, char *command
) {
  REQUEST *request = malloc(sizeof(REQUEST));

  request->pid = pid;
  request->initial_timestamp = initial_timestamp;

  struct timeval null_timestamp = {0, 0};
  request->final_timestamp = null_timestamp;

  strcpy(request->command, command);  // NOLINT

  return request;
}

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request) {
  if (requests_array->current_index == requests_array->capacity) {
    requests_array->capacity *= 2;
    requests_array->requests = realloc(
        requests_array->requests, sizeof(REQUEST *) * requests_array->capacity
    );

    if (requests_array->requests == NULL) {
      perror("realloc");
      exit(EXIT_FAILURE);
    }
  }

  requests_array->requests[requests_array->current_index] = request;
  requests_array->current_index++;
}

int insert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  int index = find_request(requests_array, info->pid);

  REQUEST *new_request = create_request(info->pid, info->timestamp, info->name);
  append_request(requests_array, new_request);

  char *fifo_name = malloc(sizeof(char) * 64);
  sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);
  write_to_fd(fd, NULL, 0, OK);

  close(fd);
  free(fifo_name);

  return index;
}

int update_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  int index = find_request(requests_array, info->pid);

  if (index != -1) {
    requests_array->requests[index]->final_timestamp = info->timestamp;
  }

  return index;
}

void free_requests_array(REQUESTS_ARRAY *requests_array) {
  for (int i = 0; i < requests_array->current_index; i++) {
    free(requests_array->requests[i]);
  }
}

int find_request(REQUESTS_ARRAY *requests_array, pid_t pid) {
  for (int i = 0; i < requests_array->current_index; i++) {
    if (requests_array->requests[i]->pid == pid) {
      return i;
    }
  }

  return -1;
}

int deal_with_request(
    REQUESTS_ARRAY *requests_array, void *data, REQUEST_TYPE type
) {
  int to_return = 0;

  if (type == NEW || type == PIPELINE) {
    PROGRAM_INFO *info = (PROGRAM_INFO *)data;
    printf(
        "%s request (%d) - '%s'\n", type == NEW ? "New" : "Pipeline", info->pid,
        info->name
    );
    to_return = insert_request(requests_array, info);
  } else if (type == UPDATE) {
    PROGRAM_INFO *info = (PROGRAM_INFO *)data;
    printf("Update request (%d) - '%s'\n", info->pid, info->name);
    to_return = update_request(requests_array, info);
    store_request(requests_array, info);
  } else if (type == STATUS) {
    PROGRAM_INFO *info = (PROGRAM_INFO *)data;
    printf("Status request (%d)\n", info->pid);
    pid_t pid = fork();
    if (pid == 0) {
      status_request(requests_array, info);
      exit(EXIT_SUCCESS);
    }
  } else if (type == STATS_TIME) {
    PIDS_ARR *pids_arr = (PIDS_ARR *)data;
    printf(
        "Stats time request with %d pids (%d)\n", pids_arr->n_pids,
        pids_arr->child_pid
    );
    pid_t pid = fork();
    if (pid == 0) {
      stats_time_request(pids_arr);
      exit(EXIT_SUCCESS);
    }
  } else if (type == STATS_COMMAND) {
    PIDS_ARR_WITH_PROGRAM *pids_arr_with_program =
        (PIDS_ARR_WITH_PROGRAM *)data;
    printf(
        "Stats command request for program %s with %d pids (%d)\n",
        pids_arr_with_program->program, pids_arr_with_program->pids_arr.n_pids,
        pids_arr_with_program->pids_arr.child_pid
    );
    pid_t pid = fork();
    if (pid == 0) {
      stats_command_request(pids_arr_with_program);
      exit(EXIT_SUCCESS);
    }
  } else {
    printf("Invalid request type received (%d)\n", type);
    exit(EXIT_FAILURE);
  }

  return to_return;
}

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  char *fifo_name = malloc(sizeof(char) * 64);
  sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);

  for (int i = 0; i < requests_array->current_index; i++) {
    REQUEST *request = requests_array->requests[i];

    // Is still running
    if (request->final_timestamp.tv_sec == 0 &&
        request->final_timestamp.tv_usec == 0) {
      write_to_fd(fd, request, sizeof(REQUEST), STATUS);
    }
  }

  // Send DONE to inform client that all requests were sent
  write_to_fd(fd, NULL, 0, DONE);

  free(fifo_name);
  close(fd);

  return 0;
}

ssize_t store_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  char *pid_str = malloc(sizeof(char) * 64);
  sprintf(pid_str, "%s/%d", folder, info->pid);  // NOLINT

  int fd = open_file_by_path(pid_str, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  // find the request
  int index = find_request(requests_array, info->pid);
  REQUEST *request = requests_array->requests[index];

  struct timeval initial_timeval = request->initial_timestamp;
  struct timeval final_timeval = request->final_timestamp;
  struct timeval result_timeval;
  timeval_subtract(&result_timeval, &final_timeval, &initial_timeval);

  char *data = malloc(sizeof(request->command) + sizeof(long int) + 64);

  // NOLINTBEGIN
  sprintf(
      data, "COMMAND: %s\nDURATION[ms]: %lf", request->command,
      timeval_to_ms(&result_timeval)
  );
  // NOLINTEND

  // Clean resources
  free(pid_str);

  return simple_write_to_fd(fd, data, strlen(data));
}

int stats_time_request(PIDS_ARR *pids_arr) {
  int n_pids = pids_arr->n_pids;

  int num_forks, files_per_fork;
  divide_files_per_fork(n_pids, &num_forks, &files_per_fork);

  // pipe from childs to communicate with parent
  int pipe_fd[2];
  pipe(pipe_fd);

  // Create and use num_forks forks to search for the files
  for (int i = 0; i < num_forks; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      close(pipe_fd[0]);  // Close stdin on child
      double total_time = 0;

      // Each fork will search for files_per_fork files
      for (int j = 0; j < files_per_fork; j++) {
        int index = i * files_per_fork + j;
        if (index >= n_pids) {
          break;
        }

        char *pid_str = malloc(sizeof(char) * 64);
        sprintf(pid_str, "%s/%d", folder, pids_arr->pids[index]);  // NOLINT

        int fd = open_file_by_path(pid_str, O_RDONLY, 0644);
        if (fd == -1) {
          continue;
        }

        double time = retrieve_time_from_file(fd);
        if (time != -1) {
          total_time += time;
        }

        // Clean resources
        free(pid_str);
        close(fd);
      }

      simple_write_to_fd(pipe_fd[1], &total_time, sizeof(double));

      exit(EXIT_SUCCESS);
    }
  }

  close(pipe_fd[1]);  // Close stdout on parent

  // Accumulate total time
  double total_time = 0.0;
  for (int i = 0; i < num_forks; i++) {
    double value;
    int bytes_read = read(pipe_fd[0], &value, sizeof(double));

    if (bytes_read != 0) {
      total_time += value;
    }
  }

  char *fifo_name = malloc(sizeof(char) * 64);
  sprintf(fifo_name, "tmp/%d.fifo", pids_arr->child_pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);
  write_to_fd(fd, &total_time, sizeof(double), STATS_TIME);

  return 0;
}

int stats_command_request(PIDS_ARR_WITH_PROGRAM *pids_arr_with_program) {
  int n_pids = pids_arr_with_program->pids_arr.n_pids;

  int num_forks, files_per_fork;
  divide_files_per_fork(n_pids, &num_forks, &files_per_fork);

  // pipe from childs to communicate with parent
  int pipe_fd[2];
  pipe(pipe_fd);

  // Create and use num_forks forks to search for the files
  for (int i = 0; i < num_forks; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      close(pipe_fd[0]);  // Close stdin on child
      int total_execs = 0;

      // Each fork will search for files_per_fork files
      for (int j = 0; j < files_per_fork; j++) {
        int index = i * files_per_fork + j;
        if (index >= n_pids) {
          break;
        }

        char *pid_str = malloc(sizeof(char) * 64);
        // NOLINTBEGIN
        sprintf(
            pid_str, "%s/%d", folder,
            pids_arr_with_program->pids_arr.pids[index]
        );
        // NOLINTEND

        int fd = open_file_by_path(pid_str, O_RDONLY, 0644);
        if (fd == -1) {
          continue;
        }

        char *program_name = retrieve_program_name_from_file(fd);
        if (!strcmp(program_name, pids_arr_with_program->program)) {
          total_execs++;
        }

        // Clean resources
        free(pid_str);
        close(fd);
      }

      simple_write_to_fd(pipe_fd[1], &total_execs, sizeof(int));

      exit(EXIT_SUCCESS);
    }
  }

  close(pipe_fd[1]);  // Close stdout on parent

  // Accumulate total execs
  int total_execs = 0;
  for (int i = 0; i < num_forks; i++) {
    int value;
    int bytes_read = read(pipe_fd[0], &value, sizeof(int));

    if (bytes_read != 0) {
      total_execs += value;
    }
  }

  char *fifo_name = malloc(sizeof(char) * 64);
  // NOLINTBEGIN
  sprintf(
      fifo_name, "tmp/%d.fifo", pids_arr_with_program->pids_arr.child_pid
  );  
  // NOLINTEND

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);
  write_to_fd(fd, &total_execs, sizeof(int), STATS_COMMAND);

  exit(EXIT_SUCCESS);
}
