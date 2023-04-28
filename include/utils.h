#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <sys/time.h>

#define MAIN_FIFO_NAME "tmp/monitor.fifo"

typedef enum request_type {
  NEW,
  UPDATE,
  STATUS,
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

PROGRAM_INFO *create_program_info(int pid, char *command, enum request_type type);

char *create_fifo(int pid);

void open_fifo(int *fd, char *fifo_name, int flags);

int write_to_fd(int fd, void *info, int size);

int read_from_fd(int fd, void *info, int size);

char *strdup(const char *s);

REQUESTS_ARRAY *create_requests_array(int size);

REQUEST *create_request(
    int pid, suseconds_t initial_timestamp, suseconds_t final_timestamp,
    char *command
);

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request);

int find_request(REQUESTS_ARRAY *requests_array, int pid);

int get_total_time(REQUESTS_ARRAY *requests_array, int index);

void free_requests_array(REQUESTS_ARRAY *requests_array);

#endif  // UTILS_H
