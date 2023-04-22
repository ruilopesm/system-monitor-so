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

REQ *create_request(int pid, suseconds_t initial_timestamp,
                    suseconds_t final_timestamp, char *command) {
  REQ *request = malloc(sizeof(struct request));

  request->pid = pid;
  request->initial_timestamp = initial_timestamp;
  request->final_timestamp = final_timestamp;
  strcpy(request->command, command);  // NOLINT

  return request;
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

  if (info->type == NEW) {
    REQ *new_request = create_request(info->pid, info->timestamp, 0,
                                      info->name);

    program_info *response_info =
        create_program_info(getpid(), "monitor", OK);

    char *fifo_name = malloc(sizeof(char) * 64);
    sprintf(fifo_name, "tmp/%d.fifo", info->pid);

    index = aux_add_request(requests_array, new_request);


    int fd;
    open_fifo(&fd, fifo_name, O_WRONLY);
    write_to_pipe(fd, response_info);
    close(fd);

    free(fifo_name);
  } 
  else if (info->type == UPDATE) { 
    index = aux_find_request(requests_array, info->pid);

    if (index != -1) requests_array[index]->final_timestamp = info->timestamp;

    int total_time = get_total_time(requests_array, index);
    printf("Total time: %d\n", total_time);
    printf("------------------------\n");
  }

  return index;
}

int get_total_time(REQ **requests_array, int index) {
  return requests_array[index]->final_timestamp -
         requests_array[index]->initial_timestamp;
}
