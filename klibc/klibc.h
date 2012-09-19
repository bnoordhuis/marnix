#ifndef KLIBC_H_
#define KLIBC_H_

#define isdigit(c)                                                            \
  ((c) >= '0' && (c) <= '9')

#define isspace(c)                                                            \
  ((c) == ' ' || (c) == '\n' || (c) == '\r' || (c) == '\t')

// XXX only works on i386
#define va_start(ap, arg)                                                     \
  ((ap).__arg = (unsigned long *) &(arg))

#define va_arg(ap, type)                                                      \
  (* (type *) (++(ap).__arg))

#define va_end(ap)

struct __va_list
{
  unsigned long *__arg;
};

typedef struct __va_list va_list;

long strtol(const char *nptr, char **endptr, int base);
int snprintf(char *buf, unsigned int len, const char *fmt, ...);
int vsnprintf(char *buf, unsigned int len, const char *fmt, va_list ap);
void *memcpy(void *dst, const void *src, unsigned int len);
void *memset(void *ptr, int c, unsigned int len);
void bzero(void *ptr, unsigned int len);

#endif // KLIBC_H_
