/* Extracted from GNU coreutils iopoll.c */
/* Function: iopoll */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern int
iopoll (int fdin, int fdout, bool block){
  return iopoll_internal (fdin, fdout, block, true);
}
