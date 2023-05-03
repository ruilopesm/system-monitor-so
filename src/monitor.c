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
    PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO));
    REQUEST_TYPE type = read_from_fd(fd, info, sizeof(PROGRAM_INFO));

    printf("Type: %d\n", type);
    deal_request(requests_array, info, type);

    free(info);
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
