/* Extracted from GNU coreutils iopoll.c */
/* Function: iopoll_input_ok */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern bool
iopoll_input_ok (int fdin){
  struct stat st;
  bool always_ready = fstat (fdin, &st) == 0
                      && (S_ISREG (st.st_mode)
                          || S_ISBLK (st.st_mode));
  return ! always_ready;
}
