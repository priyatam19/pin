/* Extracted from GNU coreutils id.c */
/* Function: print_user */

#include <stdio.h>

void
print_user (uid_t uid){
  struct passwd *pwd = nullptr;

  if (use_name)
    {
      pwd = getpwuid (uid);
      if (pwd == nullptr)
        {
          error (0, 0, _("cannot find name for user ID %ju"), (uintmax_t) uid);
          ok &= false;
        }
    }

  if (pwd)
    printf ("%s", pwd->pw_name);
  else
    printf ("%ju", (uintmax_t) uid);
}
