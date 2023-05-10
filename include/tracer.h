#ifndef TRACER_H
#define TRACER_H

#include "utils.h"

int execute_program(char *full_program, char **parsed_program, int monitor_fd);

int execute_status(int monitor_fd);

int execute_pipeline(
    char *pipeline, char **parsed_pipeline, int pipeline_cmds_count,
    int monitor_fd
);

int execute_stats_time(int monitor_fd, PIDS_ARR *pids_arr);

int execute_stats_command(
    int monitor_fd, PIDS_ARR_WITH_PROGRAM *pids_arr_with_program
);

int execute_stats_uniq(int monitor_fd, PIDS_ARR *pids_arr);

#endif  // TRACER_H
