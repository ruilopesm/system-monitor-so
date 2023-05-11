#include "utils.h"

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>

PROGRAM_INFO *create_program_info(
    pid_t pid, char *name, struct timeval timestamp
) {
  PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO));

  info->pid = pid;
  info->timestamp = timestamp;
  strcpy(info->name, name);  // NOLINT

  return info;
}

HEADER *create_header(REQUEST_TYPE type, size_t size) {
  HEADER *header = malloc(sizeof(HEADER));

  header->type = type;
  header->size = size;

  return header;
}

PIDS_ARR *create_pids_arr(int pids[32], int n_pids, pid_t child_pid) {
  PIDS_ARR *pids_arr = malloc(sizeof(PIDS_ARR));

  pids_arr->n_pids = n_pids;

  // Create proper array
  for (int i = 0; i < n_pids; i++) {
    pids_arr->pids[i] = pids[i];
  }

  pids_arr->child_pid = child_pid;

  return pids_arr;
}

PIDS_ARR_WITH_PROGRAM *create_pids_arr_with_program(
    PIDS_ARR pids_arr, char *program
) {
  PIDS_ARR_WITH_PROGRAM *pids_arr_with_program =
      malloc(sizeof(PIDS_ARR_WITH_PROGRAM));
  pids_arr_with_program->pids_arr = pids_arr;
  strcpy(pids_arr_with_program->program, program);  // NOLINT

  return pids_arr_with_program;
}

void make_fifo(char *fifo_name) {
  if (mkfifo(fifo_name, 0666) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }
}

char *create_fifo(pid_t pid) {
  char *fifo_name = malloc(sizeof(char) * 64);
  sprintf(fifo_name, "tmp/%d.fifo", pid);  // NOLINT

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

void close_fifo(int fd) {
  if (close(fd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }
}

ssize_t write_to_fd(int fd, void *info, size_t size, REQUEST_TYPE type) {
  HEADER *header = create_header(type, size);
  ssize_t written_bytes = write(fd, header, sizeof(HEADER));
  free(header);

  if (written_bytes == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  ssize_t info_written_bytes = 0;
  if (info) {
    info_written_bytes = write(fd, info, size);

    if (info_written_bytes == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }
  }

  return written_bytes + info_written_bytes;
}

void *read_from_fd(int fd, REQUEST_TYPE *type) {
  HEADER header;
  int read_bytes = read(fd, &header, sizeof(HEADER));

  if (read_bytes == -1 || header.type == ERROR) {
    perror("read or server error");
    exit(EXIT_FAILURE);
  }

  if (type != NULL) {
    *type = header.type;
  }

  void *data = malloc(header.size);
  read_bytes = read(fd, data, header.size);

  if (read_bytes == -1) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  return data;
}

int open_file_by_path(char *path, int flags, mode_t mode) {
  return open(path, flags, mode);
}

ssize_t simple_write_to_fd(int fd, void *info, size_t size) {
  ssize_t written_bytes = write(fd, info, size);

  if (written_bytes == -1 || written_bytes != (ssize_t)size) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  return written_bytes;
}

char *strdup(const char *s) {
  char *ptr = malloc(strlen(s) + 1);
  if (ptr == NULL) return NULL;

  strcpy(ptr, s);  // NOLINT

  return ptr;
}

// Source: https://stackoverflow.com/questions/15846762/timeval-subtract-explanation
int timeval_subtract(
    struct timeval *result, struct timeval *x, struct timeval *y
) {
  struct timeval yy = *y;
  y = &yy;

  // Perform the carry for the later subtraction by updating y
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }

  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  // Return 1 if result is negative
  return x->tv_sec < y->tv_sec;
}

double timeval_to_ms(struct timeval *time) {
  return (double)time->tv_sec * 1000.0 + (double)time->tv_usec / 1000.0;
}

void divide_files_per_fork(int num_files, int *num_forks, int *files_per_fork) {
  int max_files_per_fork = 5;

  *num_forks = (num_files + max_files_per_fork - 1) / max_files_per_fork;
  *files_per_fork = (num_files + *num_forks - 1) / *num_forks;
}

double retrieve_time_from_file(int fd) {
  char *buffer = malloc(sizeof(char) * 1024);
  double time = -1;

  while (read(fd, buffer, 1024) > 0) {
    // Search for "DURATION[ms]: " in the buffer
    char *duration_pos = strstr(buffer, "DURATION[ms]: ");
    if (duration_pos != NULL) {
      // Retrieve the duration from the line
      sscanf(duration_pos, "DURATION[ms]: %lf", &time);  // NOLINT
      break;
    }
  }

  free(buffer);

  return time;
}

char *retrieve_program_name_from_file(int fd) {
  char *buffer = malloc(sizeof(char) * 1024);
  char *program_name = NULL;

  while (read(fd, buffer, 1024) > 0) {
    // Search for "COMMAND: " in the buffer
    char *program_pos = strstr(buffer, "COMMAND: ");
    if (program_pos != NULL) {
      // Retrieve the program name from the line
      program_name = malloc(sizeof(char) * 1024);
      sscanf(program_pos, "COMMAND: %[^\n]", program_name);  // NOLINT
      break;
    }
  }

  free(buffer);

  return program_name;
}

int wprintf(const char *format, ...) {
    char buffer[1024];
    int num_written = 0;

    va_list args;
    va_start(args, format);

    int result = vsnprintf(buffer, 1024, format, args);
    if (result >= 0 && result < 1024) {
        num_written = write(1, buffer, result);
    }
    va_end(args);

    return num_written;
}
