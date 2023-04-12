#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>

#define MAIN_FIFO_NAME "tmp/main.fifo"

// TODO: Should have a timestamp attached
typedef struct {
  int pid;
  char name
      [50];  // TODO: variable length name: idk why, because a program can't access allocated memory of another program
} program_info;

char *strdup(const char *s);

#endif  // UTILS_H
