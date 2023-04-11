#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strdup(const char *s) {
  char *ptr = malloc(strlen(s) + 1);
  if (ptr == NULL) return NULL;

  strcpy(ptr, s);  // NOLINT

  return ptr;
}
