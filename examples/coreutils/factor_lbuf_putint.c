/* Extracted from GNU coreutils factor.c */
/* Function: lbuf_putint */

#include <stdio.h>

void
lbuf_putint (uintmax_t i){
  lbuf_putint_append (i, lbuf_buf + sizeof lbuf_buf);
}
