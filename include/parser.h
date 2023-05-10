#ifndef PARSER_H
#define PARSER_H

char **parse_command(char *command, int *counter, char *delim);

int *parse_pids(char **pids, int N);

#endif  // PARSER_H
