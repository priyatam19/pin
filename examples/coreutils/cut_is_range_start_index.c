/* Extracted from GNU coreutils cut.c */
/* Function: is_range_start_index */

#include <stdio.h>

inline bool
is_range_start_index (uintmax_t k){
  return k == current_rp->lo;
}
