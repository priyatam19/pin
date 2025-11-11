/* Extracted from GNU coreutils dd.c */
/* Function: interrupt_handler */

#include <ctype.h>

void
interrupt_handler (int sig){
  if (! SA_RESETHAND)
    signal (sig, SIG_DFL);
  interrupt_signal = sig;
}
