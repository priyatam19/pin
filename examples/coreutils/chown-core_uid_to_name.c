/* Extracted from GNU coreutils chown-core.c */
/* Function: uid_to_name */

#include <stdio.h>

extern char *
uid_to_name (uid_t uid){
  struct passwd *pwd = getpwuid (uid);
  return pwd ? xstrdup (pwd->pw_name) : uid_to_str (uid);
}
