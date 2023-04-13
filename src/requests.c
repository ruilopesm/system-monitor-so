#include "requests.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

struct request {
  int pid;
  suseconds_t initial_timestamp;
  suseconds_t final_timestamp;
  char command[256];
};

REQ **create_requests_array(int size) {
  REQ **requests_array = malloc(sizeof(REQ *) * size);

  for (int i = 0; i < size; i++) {
    requests_array[i] = NULL;
  }

  return requests_array;
}

int aux_add_request(REQ **requests_array, REQ *request) {
  int i = 0;
  while (requests_array[i] != NULL) {
    i++;
  }
  requests_array[i] = request;
  return i;
}

int aux_find_request(REQ **requests_array, int pid) {
  int i = 0;
  while (requests_array[i] != NULL) {
    if (requests_array[i]->pid == pid) {
      return i;
    }
    i++;
  }
  return -1;
}

int upsert_request(REQ **requests_array, program_info *info) {
  int index = -1;

  if (info->type == EXECUTE) {
    REQ *new_request = malloc(sizeof(struct request));
    new_request->pid = info->pid;
    new_request->initial_timestamp = info->timestamp;
    new_request->final_timestamp = 0;
    strcpy(new_request->command, info->name);

    index = aux_add_request(requests_array, new_request);
  }
  else {
    index = aux_find_request(requests_array, info->pid);

    if (index != -1)
      requests_array[index]->final_timestamp = info->timestamp;

    int total_time = get_total_time(requests_array, index);
    printf("Total time: %d\n", total_time);
  }

  return index;
}

int get_total_time(REQ **requests_array, int index) {
  return requests_array[index]->final_timestamp - requests_array[index]->initial_timestamp;
}

