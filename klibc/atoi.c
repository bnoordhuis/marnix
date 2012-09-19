#include "klibc.h"

int atoi(const char *nptr)
{
  return strtol(nptr, (void *) 0, 0);
}
