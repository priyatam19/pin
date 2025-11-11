/* Extracted from GNU coreutils factor.c */
/* Function: print_factors_single */

#include <stdio.h>

void
print_factors_single (uintmax_t t1, uintmax_t t0){
  struct factors factors;

  print_uuint (make_uuint (t1, t0));
  lbuf_putc (':');

  factor (t1, t0, &factors);

  for (int j = 0; j < factors.nfactors; j++)
    for (int k = 0; k < factors.e[j]; k++)
      {
        lbuf_putc (' ');
        print_uuint (make_uuint (0, factors.p[j]));
        if (print_exponents && factors.e[j] > 1)
          {
            lbuf_putc ('^');
            lbuf_putint (factors.e[j]);
            break;
          }
      }

  if (hi (factors.plarge))
    {
      lbuf_putc (' ');
      print_uuint (factors.plarge);
    }

  lbuf_putnl ();
}
