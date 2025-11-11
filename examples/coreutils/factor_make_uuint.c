/* Extracted from GNU coreutils factor.c */
/* Function: make_uuint */

#include <stdio.h>

uuint
make_uuint (uintmax_t hi, uintmax_t lo){
  return (uuint) {{lo, hi}};
}
