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
  STATS_COMMAND,
  STATS_UNIQ,
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

typedef struct pids_arr {
  int pids[32];
  int n_pids;
  pid_t child_pid;
} PIDS_ARR;

typedef struct pids_arr_with_program {
  PIDS_ARR pids_arr;
  char program[256];
} PIDS_ARR_WITH_PROGRAM;

PROGRAM_INFO *create_program_info(
    pid_t pid, char *name, struct timeval timestamp
);

HEADER *create_header(REQUEST_TYPE type, size_t size);

PIDS_ARR *create_pids_arr(int pids[32], int n_pids, pid_t child_pid);

PIDS_ARR_WITH_PROGRAM *create_pids_arr_with_program(
    PIDS_ARR pids_arr, char *program
);

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

double timeval_to_ms(struct timeval *time);

int *parse_pids(char **pids, int N);

void divide_files_per_fork(int num_files, int *num_forks, int *files_per_fork);

double retrieve_time_from_file(int fd);

char *retrieve_program_name_from_file(int fd);

void writeString(char *string);

void writeStringInt (char *string, int x);

int wprintf(const char *format, ...);

#endif  // UTILS_H
