/* Extracted from GNU coreutils csplit.c */
/* Function: make_filename */

#include <ctype.h>

char *
make_filename (int num){
  strcpy (filename_space, prefix);
  if (suffix)
    sprintf (filename_space + strlen (prefix), suffix, num);
  else
    sprintf (filename_space + strlen (prefix), "%0*d", digits, num);
  return filename_space;
}
