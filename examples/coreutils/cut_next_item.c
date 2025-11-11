/* Extracted from GNU coreutils cut.c */
/* Function: next_item */

#include <stdio.h>

inline void
next_item (uintmax_t *item_idx){
  (*item_idx)++;
  if ((*item_idx) > current_rp->hi)
    current_rp++;
}
