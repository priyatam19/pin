/* Extracted from GNU coreutils iopoll.c */
/* Function: fwait_for_nonblocking_write */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool
fwait_for_nonblocking_write (FILE *f){
  if (! IS_EAGAIN (errno))
    /* non-recoverable write error */
    return false;

  int fd = fileno (f);
  if (fd == -1)
    goto fail;

  /* wait for the file descriptor to become writable */
  if (iopoll_internal (-1, fd, true, false) != 0)
    goto fail;

  /* successfully waited for the descriptor to become writable */
  clearerr (f);
  return true;

fail:
  errno = EAGAIN;
  return false;
}
