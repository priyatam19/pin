/* Extracted from GNU coreutils basenc.c */
/* Function: base64_required_padding */

#include <stdio.h>

int
base64_required_padding (int len){
  int partial = len % 4;
  return partial ? 4 - partial : 0;
}
