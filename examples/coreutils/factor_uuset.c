/* Extracted from GNU coreutils factor.c */
/* Function: uuset */

#include <stdio.h>

void
uuset (uintmax_t *phi, uintmax_t *plo, uuint uu){
  *phi = hi (uu);
  *plo = lo (uu);
}
