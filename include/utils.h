#ifndef UTILS_H
#define UTILS_H

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>

#define MAIN_FIFO_NAME "tmp/monitor.fifo"

typedef enum request_type {
  NEW,
  PIPELINE,
  STATUS,
  STATS_TIME,
  UPDATE,
  ERROR,
  DONE,
  OK
} REQUEST_TYPE;

typedef struct program_info {
  pid_t pid;
  char name[256];
  struct timeval timestamp;
} PROGRAM_INFO;

typedef struct header {
  REQUEST_TYPE type;
  size_t size;
} HEADER;

PROGRAM_INFO *create_program_info(
    pid_t pid, char *name, struct timeval timestamp
);

HEADER *create_header(REQUEST_TYPE type, size_t size);

void make_fifo(char *fifo_name);

char *create_fifo(pid_t pid);

void open_fifo(int *fd, char *fifo_name, int flags);

void close_fifo(int fd);

ssize_t write_to_fd(int fd, void *info, size_t size, REQUEST_TYPE type);

void *read_from_fd(int fd, REQUEST_TYPE *type);

int open_file_by_path(char *path, int flags, mode_t mode);

ssize_t simple_write_to_fd(int fd, void *info, size_t size);

char *strdup(const char *s);

int timeval_subtract(
    struct timeval *result, struct timeval *x, struct timeval *y
);

int *parse_pids(char **pids, int N);

void divide_files_per_fork(int num_files, int *num_forks, int *files_per_fork);

int retrieve_time_from_file(int fd);

#endif  // UTILS_H
