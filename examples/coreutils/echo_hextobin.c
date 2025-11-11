/* Extracted from GNU coreutils echo.c */
/* Function: hextobin */

#include <stdio.h>

int
hextobin (unsigned char c){
  switch (c)
    {
    default: return c - '0';
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    }
}
