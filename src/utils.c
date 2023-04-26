#include "utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

PROGRAM_INFO *create_program_info(int pid, char *name, enum request_type type) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  PROGRAM_INFO *info = malloc(sizeof(PROGRAM_INFO));

  info->pid = pid;
  info->timestamp = tv.tv_usec;
  info->type = type;
  strcpy(info->name, name);  // NOLINT

  return info;
}

char *create_fifo(int pid) {
  const int MAX_SIZE = 64;
  char *fifo_name = malloc(sizeof(char) * MAX_SIZE);

  int bytes_written =
      snprintf(fifo_name, MAX_SIZE, "tmp/%d.fifo", pid);  // NOLINT
  if (bytes_written < 0 || bytes_written >= MAX_SIZE) {
    perror("snprintf");
    exit(EXIT_FAILURE);
  }

  if (mkfifo(fifo_name, 0666) == -1) {
    perror("mkfifo");
    exit(EXIT_FAILURE);
  }

  return fifo_name;
}

void open_fifo(int *fd, char *fifo_name, int flags) {
  *fd = open(fifo_name, flags);

  if (*fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }
}

int write_to_fd(int fd, PROGRAM_INFO *info) {
  int write_bytes = write(fd, info, sizeof(PROGRAM_INFO));

  if (write_bytes == -1) {
    perror("write");
    exit(EXIT_FAILURE);
  }

  return write_bytes;
}

int read_from_fd(int fd, PROGRAM_INFO *info) {
  int read_bytes = read(fd, info, sizeof(PROGRAM_INFO));

  if (read_bytes == -1 || info->type == ERROR) {
    perror("read");
    exit(EXIT_FAILURE);
  }

  return read_bytes;
}

char *strdup(const char *s) {
  char *ptr = malloc(strlen(s) + 1);
  if (ptr == NULL) return NULL;

  strcpy(ptr, s);  // NOLINT

  return ptr;
}
