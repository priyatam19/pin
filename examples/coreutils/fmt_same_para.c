/* Extracted from GNU coreutils fmt.c */
/* Function: same_para */

#include <ctype.h>
#include <stdio.h>

bool
same_para (int c){
  return (next_prefix_indent == prefix_indent
          && in_column >= next_prefix_indent + prefix_full_length
          && c != '\n' && c != EOF);
}
