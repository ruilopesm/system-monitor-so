#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

REQUESTS_ARRAY *create_requests_array(int size);

REQUEST *create_request(
    int pid, suseconds_t initial_timestamp, suseconds_t final_timestamp,
    char *command
);

void append_request(REQUESTS_ARRAY *requests_array, REQUEST *request);

int find_request(REQUESTS_ARRAY *requests_array, int pid);

int upsert_request(REQUESTS_ARRAY *requests_array, REQUEST_DATA *data);

int get_total_time(REQUESTS_ARRAY *requests_array, int index);

void free_requests_array(REQUESTS_ARRAY *requests_array);

#endif  // REQUESTS_H
