/* Extracted from GNU coreutils join.c */
/* Function: set_join_field */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
set_join_field (ptrdiff_t *var, idx_t val){
  if (0 <= *var && *var != val)
    error (EXIT_FAILURE, 0,
           _("incompatible join fields %td, %td"), *var, val);
  *var = val;
}
