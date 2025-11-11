/* Extracted from GNU coreutils install.c */
/* Function: extra_mode */

#include <stdio.h>

bool
extra_mode (mode_t input){
  mode_t mask = S_IRWXUGO | S_IFMT;
  return !! (input & ~ mask);
}
