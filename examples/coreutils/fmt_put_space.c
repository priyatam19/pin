/* Extracted from GNU coreutils fmt.c */
/* Function: put_space */

#include <ctype.h>
#include <stdio.h>

void
put_space (int space){
  int space_target, tab_target;

  space_target = out_column + space;
  if (tabs)
    {
      tab_target = space_target / TABWIDTH * TABWIDTH;
      if (out_column + 1 < tab_target)
        while (out_column < tab_target)
          {
            putchar ('\t');
            out_column = (out_column / TABWIDTH + 1) * TABWIDTH;
          }
    }
  while (out_column < space_target)
    {
      putchar (' ');
      out_column++;
    }
}
