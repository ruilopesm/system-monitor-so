#include "requests.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

REQUESTS_ARRAY *create_requests_array(int size) {
  REQUESTS_ARRAY *requests_array = malloc(sizeof(REQUESTS_ARRAY));

  requests_array->requests = malloc(sizeof(REQUEST *) * size);
  requests_array->current_index = 0;
  requests_array->capacity = size;

  return requests_array;
}

REQUEST *create_request(
    int pid, suseconds_t initial_timestamp, suseconds_t final_timestamp,
    char *command
) {
  REQUEST *request = malloc(sizeof(REQUEST));

  request->pid = pid;
  request->initial_timestamp = initial_timestamp;
  request->final_timestamp = final_timestamp;
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

int find_request(REQUESTS_ARRAY *requests_array, int pid) {
  for (int i = 0; i < requests_array->current_index; i++) {
    if (requests_array->requests[i]->pid == pid) {
      return i;
    }
  }

  return -1;
}

int upsert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  int index = find_request(requests_array, info->pid);

  if (info->type == NEW) {
    REQUEST *new_request =
        create_request(info->pid, info->timestamp, 0, info->name);
    append_request(requests_array, new_request);

    PROGRAM_INFO *response_info = create_program_info(getpid(), "monitor", OK);

    char *fifo_name = malloc(sizeof(char) * 32);
    sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

    int fd;
    open_fifo(&fd, fifo_name, O_WRONLY);
    write_to_fd(fd, response_info);
    close(fd);
    free(fifo_name);
  } else if (info->type == UPDATE) {
    if (index != -1) {
      requests_array->requests[index]->final_timestamp = info->timestamp;
    }

    int total_time = get_total_time(requests_array, index);
    printf("Total time: %d\n", total_time);
    printf("------------------------\n");
  }

  return index;
}

int get_total_time(REQUESTS_ARRAY *requests_array, int index) {
  return requests_array->requests[index]->final_timestamp -
         requests_array->requests[index]->initial_timestamp;
}
