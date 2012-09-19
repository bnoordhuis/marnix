#include "klibc.h"

void bzero(void *ptr, unsigned int len)
{
  memset(ptr, 0, len);
}
