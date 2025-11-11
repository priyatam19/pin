/* Extracted from GNU coreutils expr.c */
/* Function: eval6 */

#include <stdio.h>

VALUE *
eval6 (bool evaluate){
  VALUE *l;
  VALUE *r;
  VALUE *v;
  VALUE *i1;
  VALUE *i2;

#ifdef EVAL_TRACE
  trace ("eval6");
#endif
  if (nextarg ("+"))
    {
      require_more_args ();
      return str_value (*args++);
    }
  else if (nextarg ("length"))
    {
      r = eval6 (evaluate);
      tostring (r);
      v = int_value (mbslen (r->u.s));
      freev (r);
      return v;
    }
  else if (nextarg ("match"))
    {
      l = eval6 (evaluate);
      r = eval6 (evaluate);
      if (evaluate)
        {
          v = docolon (l, r);
          freev (l);
        }
      else
        v = l;
      freev (r);
      return v;
    }
  else if (nextarg ("index"))
    {
      size_t pos;

      l = eval6 (evaluate);
      r = eval6 (evaluate);
      tostring (l);
      tostring (r);
      pos = mbs_logical_cspn (l->u.s, r->u.s);
      v = int_value (pos);
      freev (l);
      freev (r);
      return v;
    }
  else if (nextarg ("substr"))
    {
      l = eval6 (evaluate);
      i1 = eval6 (evaluate);
      i2 = eval6 (evaluate);
      tostring (l);

      if (!toarith (i1) || !toarith (i2))
        v = str_value ("");
      else
        {
          size_t pos = getsize (i1->u.i);
          size_t len = getsize (i2->u.i);

          char *s = mbs_logical_substr (l->u.s, pos, len);
          v = str_value (s);
          free (s);
        }
      freev (l);
      freev (i1);
      freev (i2);
      return v;
    }
  else
    return eval7 (evaluate);
}
