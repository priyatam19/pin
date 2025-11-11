/* Extracted from GNU coreutils expr.c */
/* Function: eval4 */

#include <stdio.h>

VALUE *
eval4 (bool evaluate){
  VALUE *l;
  VALUE *r;
  enum { multiply, divide, mod } fxn;

#ifdef EVAL_TRACE
  trace ("eval4");
#endif
  l = eval5 (evaluate);
  while (true)
    {
      if (nextarg ("*"))
        fxn = multiply;
      else if (nextarg ("/"))
        fxn = divide;
      else if (nextarg ("%"))
        fxn = mod;
      else
        return l;
      r = eval5 (evaluate);
      if (evaluate)
        {
          if (!toarith (l) || !toarith (r))
            error (EXPR_INVALID, 0, _("non-integer argument"));
          if (fxn != multiply && mpz_sgn (r->u.i) == 0)
            error (EXPR_INVALID, 0, _("division by zero"));
          ((fxn == multiply ? mpz_mul
            : fxn == divide ? mpz_tdiv_q
            : mpz_tdiv_r)
           (l->u.i, l->u.i, r->u.i));
        }
      freev (r);
    }
}
