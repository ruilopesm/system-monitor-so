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

typedef struct request {
  int pid;
  suseconds_t initial_timestamp;
  suseconds_t final_timestamp;
  char command[256];
} REQUEST;

typedef struct REQUESTS_ARRAY {
  REQUEST **requests;
  int current_index;
  int capacity;
} REQUESTS_ARRAY;

typedef struct program_info {
  enum request_type type;
  int pid;
  char name[50];
  suseconds_t timestamp;
} PROGRAM_INFO;

typedef struct request_data {
  enum request_type type;
  union data {
    PROGRAM_INFO info;
    REQUESTS_ARRAY requests_array;
  } data;
} REQUEST_DATA;

// TODO: make a create_request_data function -> done
// TODO: apply the function above to all the functions that currently create a PROGRAM_INFO
PROGRAM_INFO *create_program_info(int pid, char *command, REQUEST_TYPE type);

REQUEST_DATA *create_request_data(enum request_type type, void *data);

char *create_fifo(int pid);

void open_fifo(int *fd, char *fifo_name, int flags);

int write_to_fd(int fd, REQUEST_DATA *data);

int read_from_fd(int fd, REQUEST_DATA *data);

char *strdup(const char *s);

#endif  // UTILS_H
