#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

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

REQUESTS_ARRAY *create_requests_array(int size);

REQUEST *create_request(
    int pid, suseconds_t initial_timestamp, suseconds_t final_timestamp,
    char *command
);

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request);

int find_request(REQUESTS_ARRAY *requests_array, int pid);

int upsert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int get_total_time(REQUESTS_ARRAY *requests_array, int index);

void free_requests_array(REQUESTS_ARRAY *requests_array);

#endif  // REQUESTS_H
