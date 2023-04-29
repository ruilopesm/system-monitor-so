#include "utils.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

PROGRAM_INFO *create_program_info(int pid, char *name) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO));

  info->pid = pid;
  info->timestamp = tv.tv_usec;
  strcpy(info->name, name);  // NOLINT

  return info;
}

HEADER *create_header(enum request_type type, size_t size) {
  HEADER *header = malloc(sizeof(HEADER));

  header->type = type;
  header->size = size;

  return header;
}

char *create_fifo(int pid) {
  const int MAX_SIZE = 64;
  char *fifo_name = malloc(sizeof(char) * MAX_SIZE);

  int bytes_written =
      snprintf(fifo_name, MAX_SIZE, "tmp/%d.fifo", pid);  // NOLINT
  if (bytes_written < 0 || bytes_written >= MAX_SIZE) {
    perror("snprintf");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(fifo_name, 0666) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }

  return fifo_name;
}

void open_fifo(int *fd, char *fifo_name, int flags) {
  *fd = open(fifo_name, flags);

  if (*fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
}

int write_to_fd(int fd, void *info, size_t size, enum request_type type) {
  int write_bytes;
  HEADER *header = create_header(type, size);

  write_bytes = write(fd, header, sizeof(HEADER));

  if (write_bytes == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  write_bytes = write(fd, info, size);

  if (write_bytes == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  free(header);

  return write_bytes;
}

enum request_type read_from_fd(int fd, void *info, size_t size) {
  int read_bytes;

  HEADER header;
  read_bytes = read(fd, &header, sizeof(HEADER));

  if (read_bytes == -1 || header.type == ERROR) {
    perror("server_error");
    exit(EXIT_FAILURE);
  }

  read_bytes = read(fd, info, size);

  if (read_bytes == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  return header.type;
}

char *strdup(const char *s) {
  char *ptr = malloc(strlen(s) + 1);
  if (ptr == NULL) return NULL;

  strcpy(ptr, s);  // NOLINT

  return ptr;
}

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

int get_total_time(REQUESTS_ARRAY *requests_array, int index) {
  return requests_array->requests[index]->final_timestamp -
         requests_array->requests[index]->initial_timestamp;
}

void free_requests_array(REQUESTS_ARRAY *requests_array) {
  for (int i = 0; i < requests_array->current_index; i++) {
    free(requests_array->requests[i]);
  }

  free(requests_array->requests);
  free(requests_array);
}
