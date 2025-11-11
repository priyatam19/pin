/* Extracted from GNU coreutils factor.c */
/* Function: print_uuint */

#include <stdio.h>

void
print_uuint (uuint t){
  uintmax_t t1 = hi (t), t0 = lo (t);
  char *bufend = lbuf_buf + sizeof lbuf_buf;

  while (t1)
    {
      uintmax_t r = t1 % BIG_POWER_OF_10;
      t1 /= BIG_POWER_OF_10;
      udiv_qrnnd (t0, r, r, t0, BIG_POWER_OF_10);
      for (int i = 0; i < LOG_BIG_POWER_OF_10; i++)
        {
          *--bufend = '0' + r % 10;
          r /= 10;
        }
    }

  lbuf_putint_append (t0, bufend);
}
