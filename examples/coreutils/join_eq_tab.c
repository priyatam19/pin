/* Extracted from GNU coreutils join.c */
/* Function: eq_tab */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool
eq_tab (mcel_t g){
  return mcel_cmp (g, tab) == 0;
}
