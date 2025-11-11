/* Extracted from GNU coreutils date.c */
/* Function: res_width */

#include <stdio.h>

ATTRIBUTE_CONST
int
res_width (long int res){
  int i = 9;
  for (long long int r = 1; (r *= 10) <= res; )
    i--;
  return i;
}
