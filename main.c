#include "fastlog.h"

#include <stdio.h>
#include <time.h>

char *test_strings[] = {
  "Apple",
  "Syzygy",
  "Turmeric",
  "IBM",
  "Careful naming communicates intent, but intent can differ from implementation.  There's no substitute for careful reading.",
  "Prince Rupert's drop",
};


int main() {
  time_t start = time(0);

  size_t bytes = 0;
  for (int i = 1e6; i > 0; i--) {
    char *msg = test_strings[i % 6];
    fastlog(msg);
    bytes += sizeof(msg);
  }

  printf("Processed about %.2f char/ms\n", bytes / (0.0 + time(0) - start));
  return 0;
}
