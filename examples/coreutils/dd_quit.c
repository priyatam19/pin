/* Extracted from GNU coreutils dd.c */
/* Function: quit */

#include <ctype.h>

void
quit (int code){
  finish_up ();
  exit (code);
}
