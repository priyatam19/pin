/* Extracted from GNU coreutils copy.c */
/* Function: errno_unsupported */

#include <stdio.h>

bool
errno_unsupported (int err){
  return err == ENOTSUP || err == ENODATA;
}
