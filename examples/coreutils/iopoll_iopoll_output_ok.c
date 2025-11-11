/* Extracted from GNU coreutils iopoll.c */
/* Function: iopoll_output_ok */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern bool
iopoll_output_ok (int fdout){
  return isapipe (fdout) > 0;
}
