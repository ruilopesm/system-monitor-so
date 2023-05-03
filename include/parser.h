#ifndef PARSER_H
#define PARSER_H

char **parse_command(char *command);

int parse_pipeline(char *pipeline, char *pipeline_cmds[2]);

#endif  // PARSER_H
