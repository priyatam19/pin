/* Extracted from GNU coreutils dd.c */
/* Function: ifsync */

#include <ctype.h>

int
ifsync (int fd){
  int ret;

  do
    {
      process_signals ();
      ret = fsync (fd);
    }
  while (ret < 0 && errno == EINTR);

  return ret;
}
