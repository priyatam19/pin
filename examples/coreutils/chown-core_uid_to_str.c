/* Extracted from GNU coreutils chown-core.c */
/* Function: uid_to_str */

#include <stdio.h>

char *
uid_to_str (uid_t uid){
  char buf[INT_BUFSIZE_BOUND (intmax_t)];
  return xstrdup (TYPE_SIGNED (uid_t) ? imaxtostr (uid, buf)
                  : umaxtostr (uid, buf));
}
