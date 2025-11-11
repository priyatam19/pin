/* Extracted from GNU coreutils copy.c */
/* Function: is_terminal_error */

#include <stdio.h>

bool
is_terminal_error (int err){
  return err == EIO || err == ENOMEM || err == ENOSPC || err == EDQUOT;
}
