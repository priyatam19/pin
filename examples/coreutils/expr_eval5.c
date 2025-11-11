/* Extracted from GNU coreutils expr.c */
/* Function: eval5 */

#include <stdio.h>

VALUE *
eval5 (bool evaluate){
  VALUE *l;
  VALUE *r;
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval5");
#endif
  l = eval6 (evaluate);
  while (true)
    {
      if (nextarg (":"))
        {
          r = eval6 (evaluate);
          if (evaluate)
            {
              v = docolon (l, r);
              freev (l);
              l = v;
            }
          freev (r);
        }
      else
        return l;
    }
}
