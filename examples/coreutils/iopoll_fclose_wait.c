/* Extracted from GNU coreutils iopoll.c */
/* Function: fclose_wait */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern bool
fclose_wait (FILE *f){
  for (;;)
    {
      if (fflush (f) == 0)
        break;

      if (! fwait_for_nonblocking_write (f))
        break;
    }

  return fclose (f) == 0;
}
