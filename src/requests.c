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

int deal_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info, enum request_type type) {
  if (type == NEW) {
    printf("New request\n");
    return insert_request(requests_array, info);
  } 
  else if (type == UPDATE) {
    printf("Update request\n");
    return update_request(requests_array, info);
  } 
  else if (type == STATUS) {
    return status_request(requests_array, info);
  } 
  else {
    perror("Invalid request type");
    exit(EXIT_FAILURE);
  }
}

int insert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  int index = find_request(requests_array, info->pid);

  REQUEST *new_request =
      create_request(info->pid, info->timestamp, 0, info->name);
  append_request(requests_array, new_request);

  PROGRAM_INFO *response_info =
      create_program_info(getpid(), "monitor");

  char *fifo_name = malloc(sizeof(char) * 32);
  sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);
  write_to_fd(fd, response_info, sizeof(PROGRAM_INFO), UPDATE);

  close(fd);
  free(fifo_name);

  return index;
}

int update_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  int index = find_request(requests_array, info->pid);

  if (index != -1) {
    requests_array->requests[index]->final_timestamp = info->timestamp;
  }

  int total_time = get_total_time(requests_array, index);
  printf("Total time: %d\n", total_time);
  printf("------------------------\n");

  return index;
}

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info) {
  char *fifo_name = malloc(sizeof(char) * 32);
  sprintf(fifo_name, "tmp/%d.fifo", info->pid);  // NOLINT

  int fd;
  open_fifo(&fd, fifo_name, O_WRONLY);

  for (int i = 0; i < requests_array->current_index; i++) {
    REQUEST *request = requests_array->requests[i];

    if (request->final_timestamp == 0)
      write_to_fd(fd, request, sizeof(REQUEST), STATUS);
  }

  free(fifo_name);
  close(fd);

  printf("------------------------\n");
  return 0;
}
