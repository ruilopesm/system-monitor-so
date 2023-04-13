#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

typedef struct request REQ;

REQ *create_requests_array(int size);

int upsert_request(REQ *requests_array, program_info *info);

#endif  // REQUESTS_H
