/* Extracted from GNU coreutils chown-core.c */
/* Function: gid_to_name */

#include <stdio.h>

extern char *
gid_to_name (gid_t gid){
  struct group *grp = getgrgid (gid);
  return grp ? xstrdup (grp->gr_name) : gid_to_str (gid);
}
