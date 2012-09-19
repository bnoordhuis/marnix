#include "klibc.h"

void *memcpy(void *dst, const void *src, unsigned int len)
{
  unsigned int i;
  const char *q;
  char *p;

  q = src;
  p = dst;

  for (i = 0; i < len; i++)
    p[i] = q[i];

  return dst;
}
