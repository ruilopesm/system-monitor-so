#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

typedef struct request REQ;

REQ **create_requests_array(int size);

int upsert_request(REQ **requests_array, program_info *info);

int get_total_time(REQ **requests_array, int index);

#endif  // REQUESTS_H
