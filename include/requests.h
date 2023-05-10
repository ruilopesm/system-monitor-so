#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

typedef struct request {
  pid_t pid;
  struct timeval initial_timestamp;
  struct timeval final_timestamp;
  char command[256];
} REQUEST;

typedef struct REQUESTS_ARRAY {
  REQUEST **requests;
  int current_index;
  int capacity;
} REQUESTS_ARRAY;

int deal_with_request(
    REQUESTS_ARRAY *requests_array, void *data, REQUEST_TYPE type
);

REQUESTS_ARRAY *create_requests_array(size_t size);

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request);

int insert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int update_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

void free_requests_array(REQUESTS_ARRAY *requests_array);

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int find_request(REQUESTS_ARRAY *requests_array, pid_t pid);

REQUEST *create_request(
    pid_t pid, struct timeval initial_timestamp, char *command
);

ssize_t store_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int stats_time_request(PIDS_ARR *pids_arr, int n_pids);

#endif  // REQUESTS_H
