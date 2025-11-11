/* Extracted from GNU coreutils dd.c */
/* Function: ifdatasync */

#include <ctype.h>

int
ifdatasync (int fd){
  int ret;

  do
    {
      process_signals ();
      ret = fdatasync (fd);
    }
  while (ret < 0 && errno == EINTR);

  return ret;
}
