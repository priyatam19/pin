/* Extracted from GNU coreutils dd.c */
/* Function: siginfo_handler */

#include <ctype.h>

void
siginfo_handler (int sig){
  if (! SA_NOCLDSTOP)
    signal (sig, siginfo_handler);
  info_signal_count++;
}
