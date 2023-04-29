#ifndef REQUESTS_H
#define REQUESTS_H

#include "utils.h"

int deal_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info, enum request_type type);

int insert_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int update_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

int status_request(REQUESTS_ARRAY *requests_array, PROGRAM_INFO *info);

#endif  // REQUESTS_H
