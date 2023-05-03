#include "parser.h"

#include <stdlib.h>
#include <string.h>

char **parse_command(char *command) {
  int arg_count = 1, i = 0;
  char **arguments = malloc(sizeof(char *) * (arg_count + 1));

  char *arg = strtok(command, " ");
  while (arg != NULL) {
    arguments[i] = arg;
    i++;

    if (i >= arg_count) {
      arg_count++;
      arguments = realloc(arguments, sizeof(char *) * (arg_count + 1));
    }

    arg = strtok(NULL, " ");
  }

  arguments[i] = NULL;

  return arguments;
}

int parse_pipeline(char *pipeline, char *pipeline_cmds[2]) {
  int cmd_count = 0, i = 0;

  char *cmd = strtok(pipeline, "|");
  while (cmd != NULL) {
    pipeline_cmds[i] = cmd;
    i++;

    if (i >= cmd_count) {
      cmd_count++;
    }

    cmd = strtok(NULL, "|");
  }

  return cmd_count;
}
