/* Extracted from GNU coreutils join.c */
/* Function: newline_or_blank */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool
newline_or_blank (mcel_t g){
  return g.ch == '\n' || c32isblank (g.ch);
}
