/* Extracted from GNU coreutils expr.c */
/* Function: eval1 */

#include <stdio.h>

VALUE *
eval1 (bool evaluate){
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval1");
#endif
  l = eval2 (evaluate);
  while (true)
    {
      if (nextarg ("&"))
        {
          r = eval2 (evaluate && !null (l));
          if (null (l) || null (r))
            {
              freev (l);
              freev (r);
              l = int_value (0);
            }
          else
            freev (r);
        }
      else
        return l;
    }
}
