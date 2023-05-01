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

int deal_request(
    REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info, enum request_type type
);

REQUESTS_ARRAY *create_requests_array(int size);

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request);

int insert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int update_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

void free_requests_array(REQUESTS_ARRAY *requests_array);

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int find_request(REQUESTS_ARRAY *requests_array, int pid);

REQUEST *create_request(int pid, suseconds_t initial_timestamp, char *command);

#endif  // REQUESTS_H
