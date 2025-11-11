/* Extracted from GNU coreutils dd.c */
/* Function: iclose */

#include <ctype.h>

int
iclose (int fd){
  if (close (fd) != 0)
    do
      if (errno != EINTR)
        return -1;
    while (close (fd) != 0 && errno != EBADF);

  return 0;
}
