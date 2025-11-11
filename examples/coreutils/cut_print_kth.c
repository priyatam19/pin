/* Extracted from GNU coreutils cut.c */
/* Function: print_kth */

#include <stdio.h>

inline bool
print_kth (uintmax_t k){
  return current_rp->lo <= k;
}
