/* Extracted from GNU coreutils chroot.c */
/* Function: uid_unset */

#include <ctype.h>
#include <stdio.h>

inline bool uid_unset (uid_t uid){ return uid == (uid_t) -1; }
