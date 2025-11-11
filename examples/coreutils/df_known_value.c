/* Extracted from GNU coreutils df.c */
/* Function: known_value */

#include <stdio.h>

bool
known_value (uintmax_t n){
  return n < UINTMAX_MAX - 1;
}
