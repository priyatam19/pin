/* Extracted from GNU coreutils dd.c */
/* Function: multiple_bits_set */

#include <ctype.h>

inline bool
multiple_bits_set (int i){
  return MULTIPLE_BITS_SET (i);
}
