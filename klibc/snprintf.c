#include "klibc.h"

int snprintf(char *buf, unsigned int len, const char *fmt, ...)
{
  va_list ap;
  int r;

  va_start(ap, fmt);
  r = vsnprintf(buf, len, fmt, ap);
  va_end(ap);

  return r;
}
