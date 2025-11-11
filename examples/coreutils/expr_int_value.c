/* Extracted from GNU coreutils expr.c */
/* Function: int_value */

#include <stdio.h>

VALUE *
int_value (unsigned long int i){
  VALUE *v = xmalloc (sizeof *v);
  v->type = integer;
  mpz_init_set_ui (v->u.i, i);
  return v;
}
