/* Extracted from GNU coreutils csplit.c */
/* Function: interrupt_handler */

#include <ctype.h>

void
interrupt_handler (int sig){
  delete_all_files (true);
  signal (sig, SIG_DFL);
  /* The signal has been reset to SIG_DFL, but blocked during this
     handler.  Force the default action of this signal once the
     handler returns and the block is removed.  */
  raise (sig);
}
