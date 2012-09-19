#include "klibc.h"

// FIXME we should be able to parse -2147483648 but we can't
long strtol(const char *nptr, char **endptr, int base)
{
  const char *ptr;
  int last_digit;
  int negative;
  int ndigits;
  long val;
  char c;

  last_digit = '9';
  negative = 0;
  ndigits = 0;
  ptr = nptr;
  val = 0;

  while (isspace(*ptr))
    ptr++;

  if (*ptr == '\0')
    goto out;

  if (*ptr == '-') {
    negative = 1;
    ptr += 1;
  }

  if (*ptr == '\0')
    goto out;

  if (base == 0) {
    // sniff base
    if (ptr[0] != '0')
      base = 10;
    else if (ptr[1] == 'x' || ptr[1] == 'X') {
      base = 16;
      ptr += 2;
    }
    else {
      last_digit = '8';
      base = 8;
      ptr += 1;
    }
  }

  for (; *ptr != '\0'; ptr++, ndigits++) {
    c = *ptr;

    if (c >= '0' && c <= last_digit) {
      // XXX check for overflow?
      val *= base;
      val += c - '0';
      continue;
    }

    c |= 32; // lowercase

    if (base != 16 || c < 'a' || c > 'f')
      goto out;

    val *= base;
    val += c - 'a';
  }

out:
  if (endptr)
    *endptr = (char *) (ndigits ? ptr : nptr);

  if (negative)
    val = -val;

  return val;
}
