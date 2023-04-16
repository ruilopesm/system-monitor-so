#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <sys/time.h>

#define MAIN_FIFO_NAME "/tmp/main.fifo"

enum request_type {
  EXECUTE,
  DONE
};

typedef struct {
  int pid;
  char name
      [50];  // TODO: variable length name: idk why, because a program can't access allocated memory of another program
  suseconds_t timestamp;
  enum request_type type;
} program_info;

char *strdup(const char *s);

#endif  // UTILS_H
