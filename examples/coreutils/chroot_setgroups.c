/* Extracted from GNU coreutils chroot.c */
/* Function: setgroups */

#include <ctype.h>
#include <stdio.h>

int
setgroups (size_t size, MAYBE_UNUSED gid_t const *list){
  if (size == 0)
    {
      /* Return success when clearing supplemental groups
         as ! HAVE_SETGROUPS should only be the case on
         platforms that don't support supplemental groups.  */
      return 0;
    }
  else
    {
      errno = ENOTSUP;
      return -1;
    }
}
