/* Extracted from GNU coreutils basenc.c */
/* Function: base32_required_padding */

#include <stdio.h>

int
base32_required_padding (int len){
  int partial = len % 8;
  return partial ? 8 - partial : 0;
}
