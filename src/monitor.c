#include "monitor.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

int main(void) {
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

  while (true) {
    // Read data from the named pipe
    program_info *info = malloc(sizeof(program_info));
    int read_bytes = read(fd, info, sizeof(program_info));
    if (read_bytes == -1) {
      perror("read");
      exit(EXIT_FAILURE);
    }

    if (read_bytes != 0) {
      printf("PID %d: %s\n", info->pid, info->name);
    }
  }

  // Close the named pipe
  close(fd);

  // Delete named pipe file
  if (unlink(MAIN_FIFO_NAME) == -1) {
    perror("unlink");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
