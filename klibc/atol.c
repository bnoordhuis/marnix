#include "klibc.h"

long atol(const char *nptr)
{
  return strtol(nptr, (void *) 0, 0);
}
