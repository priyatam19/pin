/* Extracted from GNU coreutils du.c */
/* Function: print_only_size */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void
print_only_size (uintmax_t n_bytes){
  char buf[LONGEST_HUMAN_READABLE + 1];
  fputs ((n_bytes == UINTMAX_MAX
          ? _("Infinity")
          : human_readable (n_bytes, buf, human_output_opts,
                            1, output_block_size)),
         stdout);
}
