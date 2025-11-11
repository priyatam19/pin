/* Extracted from GNU coreutils expr.c */
/* Function: eval7 */

#include <stdio.h>

VALUE *
eval7 (bool evaluate){
  VALUE *v;

#ifdef EVAL_TRACE
  trace ("eval7");
#endif
  require_more_args ();

  if (nextarg ("("))
    {
      v = eval (evaluate);
      if (nomoreargs ())
        error (EXPR_INVALID, 0, _("syntax error: expecting ')' after %s"),
               quotearg_n_style (0, locale_quoting_style, *(args - 1)));
      if (!nextarg (")"))
        error (EXPR_INVALID, 0, _("syntax error: expecting ')' instead of %s"),
               quotearg_n_style (0, locale_quoting_style, *args));
      return v;
    }

  if (nextarg (")"))
    error (EXPR_INVALID, 0, _("syntax error: unexpected ')'"));

  return str_value (*args++);
}
