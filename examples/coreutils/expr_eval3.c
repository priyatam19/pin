/* Extracted from GNU coreutils expr.c */
/* Function: eval3 */

#include <stdio.h>

VALUE *
eval3 (bool evaluate){
  VALUE *l;
  VALUE *r;
  enum { plus, minus } fxn;

#ifdef EVAL_TRACE
  trace ("eval3");
#endif
  l = eval4 (evaluate);
  while (true)
    {
      if (nextarg ("+"))
        fxn = plus;
      else if (nextarg ("-"))
        fxn = minus;
      else
        return l;
      r = eval4 (evaluate);
      if (evaluate)
        {
          if (!toarith (l) || !toarith (r))
            error (EXPR_INVALID, 0, _("non-integer argument"));
          (fxn == plus ? mpz_add : mpz_sub) (l->u.i, l->u.i, r->u.i);
        }
      freev (r);
    }
}
