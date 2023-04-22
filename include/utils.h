#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <sys/time.h>

#define MAIN_FIFO_NAME "tmp/main.fifo"

enum request_type {
  NEW,
  UPDATE,
  ERROR,
  OK
};

typedef struct {
  enum request_type type;
  int pid;
  char name[50];
  suseconds_t timestamp;
} program_info;

char *strdup(const char *s);

program_info *create_program_info(int pid, char *name, enum request_type type);

char* create_fifo(int pid);

void open_fifo(int *fd, char *fifo_name, int flags);

int write_to_pipe(int fd, program_info *info);

int read_from_pipe(int fd, program_info *info);

#endif  // UTILS_H
