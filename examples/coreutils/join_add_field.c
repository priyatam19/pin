/* Extracted from GNU coreutils join.c */
/* Function: add_field */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
add_field (int file, idx_t field){
  struct outlist *o;

  affirm (file == 0 || file == 1 || file == 2);
  affirm (file != 0 || field == 0);

  o = xmalloc (sizeof *o);
  o->file = file;
  o->field = field;
  o->next = nullptr;

  /* Add to the end of the list so the fields are in the right order.  */
  outlist_end->next = o;
  outlist_end = o;
}
