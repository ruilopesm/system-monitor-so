#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

int deal_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int upsert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

#endif  // REQUESTS_H
