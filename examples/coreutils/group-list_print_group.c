/* Extracted from GNU coreutils group-list.c */
/* Function: print_group */

#include <stdio.h>

extern bool
print_group (gid_t gid, bool use_name){
  struct group *grp = nullptr;
  bool ok = true;

  if (use_name)
    {
      grp = getgrgid (gid);
      if (grp == nullptr)
        {
          if (TYPE_SIGNED (gid_t))
            {
              intmax_t g = gid;
              error (0, 0, _("cannot find name for group ID %jd"), g);
            }
          else
            {
              uintmax_t g = gid;
              error (0, 0, _("cannot find name for group ID %ju"), g);
            }
          ok = false;
        }
    }

  if (grp)
    printf ("%s", grp->gr_name);
  else
    printf ("%ju", (uintmax_t) gid);
  return ok;
}
