/* Extracted from GNU coreutils expr.c */
/* Function: eval2 */

#include <stdio.h>

VALUE *
eval2 (bool evaluate){
  VALUE *l;

#ifdef EVAL_TRACE
  trace ("eval2");
#endif
  l = eval3 (evaluate);
  while (true)
    {
      VALUE *r;
      enum
        {
          less_than, less_equal, equal, not_equal, greater_equal, greater_than
        } fxn;
      bool val = false;

      if (nextarg ("<"))
        fxn = less_than;
      else if (nextarg ("<="))
        fxn = less_equal;
      else if (nextarg ("=") || nextarg ("=="))
        fxn = equal;
      else if (nextarg ("!="))
        fxn = not_equal;
      else if (nextarg (">="))
        fxn = greater_equal;
      else if (nextarg (">"))
        fxn = greater_than;
      else
        return l;
      r = eval3 (evaluate);

      if (evaluate)
        {
          int cmp;
          tostring (l);
          tostring (r);

          if (looks_like_integer (l->u.s) && looks_like_integer (r->u.s))
            cmp = strintcmp (l->u.s, r->u.s);
          else
            {
              errno = 0;
              cmp = strcoll (l->u.s, r->u.s);

              if (errno)
                {
                  error (0, errno, _("string comparison failed"));
                  error (0, 0, _("set LC_ALL='C' to work around the problem"));
                  error (EXPR_INVALID, 0,
                         _("the strings compared were %s and %s"),
                         quotearg_n_style (0, locale_quoting_style, l->u.s),
                         quotearg_n_style (1, locale_quoting_style, r->u.s));
                }
            }

          switch (fxn)
            {
            case less_than:     val = (cmp <  0); break;
            case less_equal:    val = (cmp <= 0); break;
            case equal:         val = (cmp == 0); break;
            case not_equal:     val = (cmp != 0); break;
            case greater_equal: val = (cmp >= 0); break;
            case greater_than:  val = (cmp >  0); break;
            default: unreachable ();
            }
        }

      freev (l);
      freev (r);
      l = int_value (val);
    }
}
