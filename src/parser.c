#include "parser.h"

#include <stdlib.h>
#include <string.h>

char **parse_command(char *command) {
  int arg_count = 1;
  char **arguments = malloc(sizeof(char *) * (arg_count + 1));
  int i = 0;

  char *arg = strtok(command, " ");
  while (arg != NULL) {
    arguments[i] = arg;
    i++;

    if (i >= arg_count) {
      arg_count += 1;
      arguments = realloc(arguments, sizeof(char *) * (arg_count + 1));
    }

    arg = strtok(NULL, " ");
  }
  arguments[i] = NULL;

  return arguments;
}
