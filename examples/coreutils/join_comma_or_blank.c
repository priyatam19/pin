/* Extracted from GNU coreutils join.c */
/* Function: comma_or_blank */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool
comma_or_blank (mcel_t g){
  return g.ch == ',' || c32isblank (g.ch);
}
