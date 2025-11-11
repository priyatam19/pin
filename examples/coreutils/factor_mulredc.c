/* Extracted from GNU coreutils factor.c */
/* Function: mulredc */

#include <stdio.h>

inline uintmax_t
mulredc (uintmax_t a, uintmax_t b, uintmax_t m, uintmax_t mi){
  uintmax_t rh, rl, q, th, xh;
  MAYBE_UNUSED uintmax_t tl;

  umul_ppmm (rh, rl, a, b);
  q = rl * mi;
  umul_ppmm (th, tl, q, m);
  xh = rh - th;
  if (rh < th)
    xh += m;

  return xh;
}
