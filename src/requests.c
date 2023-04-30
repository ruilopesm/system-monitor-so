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

REQUEST *create_request(int pid, suseconds_t initial_timestamp, char *command) {
  REQUEST *request = malloc(sizeof(REQUEST));

  request->pid = pid;
  request->initial_timestamp = initial_timestamp;
  request->final_timestamp = 0;
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

  char *fifo_name = malloc(sizeof(char) * 32);
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

int find_request(REQUESTS_ARRAY *requests_array, int pid) {
  for (int i = 0; i < requests_array->current_index; i++) {
    if (requests_array->requests[i]->pid == pid) {
      return i;
    }
  }

  return -1;
}

int deal_request(
    REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info, enum request_type type
) {
  if (type == NEW) {
    printf("New request\n");
    return insert_request(requests_array, info);
  } else if (type == UPDATE) {
    printf("Update request\n");
    return update_request(requests_array, info);
  } else if (type == STATUS) {
    return status_request(requests_array, info);
  } else {
    perror("Invalid request type");
    exit(EXIT_FAILURE);
  }
}

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  char *fifo_name = malloc(sizeof(char) * 32);
  sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);

  for (int i = 0; i < requests_array->current_index; i++) {
    REQUEST *request = requests_array->requests[i];

    if (request->final_timestamp == 0)
      write_to_fd(fd, request, sizeof(REQUEST), STATUS);
  }

  // Send DONE to inform client that all requests were sent
  write_to_fd(fd, NULL, 0, DONE);

  free(fifo_name);
  close(fd);

  printf("------------------------\n");
  return 0;
}
