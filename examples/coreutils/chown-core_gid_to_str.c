/* Extracted from GNU coreutils chown-core.c */
/* Function: gid_to_str */

#include <stdio.h>

char *
gid_to_str (gid_t gid){
  char buf[INT_BUFSIZE_BOUND (intmax_t)];
  return xstrdup (TYPE_SIGNED (gid_t) ? imaxtostr (gid, buf)
                  : umaxtostr (gid, buf));
}
