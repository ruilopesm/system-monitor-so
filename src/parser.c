#include "parser.h"

#include <stdlib.h>
#include <string.h>

char **parse_command(char *command, int *counter, char *delim) {
  char **parsed_command = malloc(sizeof(char *));
  int parsed_command_size = 0;

  char *token = strtok(command, delim);

  while (token != NULL) {
    parsed_command[parsed_command_size++] = token;
    parsed_command =
        realloc(parsed_command, sizeof(char *) * (parsed_command_size + 1));

    token = strtok(NULL, delim);
  }

  parsed_command[parsed_command_size] = NULL;

  if (counter) {
    *counter = parsed_command_size;
  }

  return parsed_command;
}
