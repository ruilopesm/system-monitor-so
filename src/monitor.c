#include "monitor.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "requests.h"
#include "utils.h"

int main(void) {
  REQUESTS_ARRAY *requests_array = create_requests_array(100);

  if (mkfifo(MAIN_FIFO_NAME, 0666) ==
      -1) {  // 0666 stands for read/write permissions for all users
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }

  printf("Monitor is running...\n");

  // Open the named pipe for reading
  int fd = open(MAIN_FIFO_NAME, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  // Open the named pipe for writing, in order to avoid the server being on hold
  int fd2 = open(MAIN_FIFO_NAME, O_WRONLY);
  if (fd2 == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  while (true) {
    // Read data from the named pipe
    /* PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO)); */
    REQUEST_DATA *request_data = malloc(sizeof(REQUEST_DATA));
    int read_bytes = read(fd, request_data, sizeof(REQUEST_DATA));
    if (read_bytes == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    PROGRAM_INFO info = request_data->data.info;

    if (read_bytes != 0) {
      printf("PID %d: %s\n", info.pid, info.name);
      printf("Timestamp: %ld\n", info.timestamp);
      printf("Type: %d\n", request_data->type);

      upsert_request(requests_array, request_data);
    }

    free(request_data);
  }

  // Close the named pipes
  if (close(fd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  if (close(fd2) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  // Delete named pipe file
  if (unlink(MAIN_FIFO_NAME) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
