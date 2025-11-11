/* Extracted from GNU coreutils dd.c */
/* Function: advance_input_offset */

#include <ctype.h>

void
advance_input_offset (intmax_t offset){
  if (0 <= input_offset && ckd_add (&input_offset, input_offset, offset))
    input_offset = -1;
}
