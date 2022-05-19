#include <fcntl.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const char logname[] = "/tmp/fastlog.log";

int fastlog(const char* msg) {
  if (!msg || !*msg)
    return -1;

  // Iterate through the string.  If it has the special value '?', then ignore it
  const char *str = msg;
  while (*str++) {
    if ('?' == *str)
      return -1;
  }

  int fd = open(logname, O_RDWR | O_APPEND);
  size_t msg_sz = 0;
  if (-1 == fd)
    return -1;
  if (1 < write(fd, msg, msg_sz))
    return -1;

  return 0;
}
