/* Extracted from GNU coreutils chroot.c */
/* Function: gid_unset */

#include <ctype.h>
#include <stdio.h>

inline bool gid_unset (gid_t gid){ return gid == (gid_t) -1; }
