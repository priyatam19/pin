/* Extracted from GNU coreutils factor.c */
/* Function: hiset */

#include <stdio.h>

void hiset (uuint *u, uintmax_t hi){ u->uu[1] = hi; }
