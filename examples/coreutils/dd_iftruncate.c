/* Extracted from GNU coreutils dd.c */
/* Function: iftruncate */

#include <ctype.h>

int
iftruncate (int fd, off_t length){
  int ret;

  do
    {
      process_signals ();
      ret = ftruncate (fd, length);
    }
  while (ret < 0 && errno == EINTR);

  return ret;
}
