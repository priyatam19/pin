/* Extracted from GNU coreutils expr.c */
/* Function: eval */

#include <stdio.h>

VALUE *
eval (bool evaluate){
  VALUE *l;
  VALUE *r;

#ifdef EVAL_TRACE
  trace ("eval");
#endif
  l = eval1 (evaluate);
  while (true)
    {
      if (nextarg ("|"))
        {
          r = eval1 (evaluate && null (l));
          if (null (l))
            {
              freev (l);
              l = r;
              if (null (l))
                {
                  freev (l);
                  l = int_value (0);
                }
            }
          else
            freev (r);
        }
      else
        return l;
    }
}
