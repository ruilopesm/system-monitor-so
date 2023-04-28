#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <sys/time.h>

#define MAIN_FIFO_NAME "tmp/monitor.fifo"

typedef enum request_type {
  NEW,
  UPDATE,
  ERROR,
  OK
} REQUEST_TYPE;

typedef struct program_info {
  enum request_type type;
  int pid;
  char name[50];
  suseconds_t timestamp;
} PROGRAM_INFO;

PROGRAM_INFO *create_program_info(
    int pid, char *command, suseconds_t timestamp, REQUEST_TYPE type
);

char *create_fifo(int pid);

void open_fifo(int *fd, char *fifo_name, int flags);

int write_to_fd(int fd, PROGRAM_INFO *info);

int read_from_fd(int fd, PROGRAM_INFO *info);

char *strdup(const char *s);

int timeval_subtract(
    struct timeval *result, struct timeval *x, struct timeval *y
);

#endif  // UTILS_H
