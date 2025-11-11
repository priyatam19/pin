/* Extracted from GNU coreutils basename.c
 * Function: remove_suffix
 * Purpose: Remove SUFFIX from the end of NAME if it is there,
 *          unless NAME consists entirely of SUFFIX.
 */

#include <string.h>

void remove_suffix(char *name, char const *suffix) {
  char *np;
  char const *sp;

  np = name + strlen(name);
  sp = suffix + strlen(suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;
  if (np > name)
    *np = '\0';
}
