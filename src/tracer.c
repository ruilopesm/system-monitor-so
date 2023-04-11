#include "tracer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

int main(void) {
  int fd;
  do {
    fd = open(MAIN_FIFO_NAME, O_WRONLY);
    if (fd == -1) {
      perror("open");
      sleep(1);
    }
  } while (fd == -1);

  // Write data to the named pipe
  if (write(fd, "Hello, monitor!", 15) == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  // Close the named pipe
  close(fd);

  exit(EXIT_SUCCESS);
}
