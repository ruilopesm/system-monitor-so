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

// Default folder
char *folder = "PIDS-folder";

int main(int argc, char **argv) {
  if (argc == 2) {
    // Folder has been specified
    folder = argv[1];
  } else if (argc > 2) {
    printf("Usage: %s <folder>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Create the folder if it doesn't exist
  if (access(folder, F_OK) == -1) {
    if (mkdir(folder, 0777) == -1) {
      perror("mkdir");
      exit(EXIT_FAILURE);
    }
  }

  // Create the named pipe
  make_fifo(MAIN_FIFO_NAME);

  printf("Monitor is running...\n");

  // Open the named pipe for reading
  int fd;
  open_fifo(&fd, MAIN_FIFO_NAME, O_RDONLY);

  // Open the named pipe for writing, in order to avoid the server being on hold
  int fd2;
  open_fifo(&fd2, MAIN_FIFO_NAME, O_WRONLY);

  // Create the requests array with initial size of 100
  REQUESTS_ARRAY *requests_array = create_requests_array(100);

  while (true) {
    // Read data from the named pipe
    PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO));
    REQUEST_TYPE type = read_from_fd(fd, info, sizeof(PROGRAM_INFO));

    deal_with_request(requests_array, info, type);

    free(info);
  }

  // Close the named pipes
  close_fifo(fd);
  close_fifo(fd2);

  // Delete named pipe file
  if (unlink(MAIN_FIFO_NAME) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
