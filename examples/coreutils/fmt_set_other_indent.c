/* Extracted from GNU coreutils fmt.c */
/* Function: set_other_indent */

#include <ctype.h>
#include <stdio.h>

void
set_other_indent (bool same_paragraph){
  if (split)
    other_indent = first_indent;
  else if (crown)
    {
      other_indent = (same_paragraph ? in_column : first_indent);
    }
  else if (tagged)
    {
      if (same_paragraph && in_column != first_indent)
        {
          other_indent = in_column;
        }

      /* Only one line: use the secondary indent from last time if it
         splits, or 0 if there have been no multi-line paragraphs in the
         input so far.  But if these rules make the two indents the same,
         pick a new secondary indent.  */

      else if (other_indent == first_indent)
        other_indent = first_indent == 0 ? DEF_INDENT : 0;
    }
  else
    {
      other_indent = first_indent;
    }
}
