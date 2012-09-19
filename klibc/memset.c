#include "klibc.h"

void *memset(void *ptr, int c, unsigned int len)
{
  unsigned int i;
  char *p;

  p = ptr;

  for (i = 0; i < len; i++)
    p[i] = c;

  return ptr;
}
