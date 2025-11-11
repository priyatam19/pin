/* Extracted from GNU coreutils dd.c */
/* Function: cache_round */

#include <ctype.h>

off_t
cache_round (int fd, off_t len){
  static off_t i_pending, o_pending;
  off_t *pending = (fd == STDIN_FILENO ? &i_pending : &o_pending);

  if (len)
    {
      intmax_t c_pending;
      if (ckd_add (&c_pending, *pending, len))
        c_pending = INTMAX_MAX;
      *pending = c_pending % IO_BUFSIZE;
      if (c_pending > *pending)
        len = c_pending - *pending;
      else
        len = 0;
    }
  else
    len = *pending;

  return len;
}
