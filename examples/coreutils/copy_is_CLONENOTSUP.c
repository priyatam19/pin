/* Extracted from GNU coreutils copy.c */
/* Function: is_CLONENOTSUP */

#include <stdio.h>

bool
is_CLONENOTSUP (int err){
  return err == ENOSYS || err == ENOTTY || is_ENOTSUP (err)
         || err == EINVAL || err == EBADF
         || err == EXDEV || err == ETXTBSY
         || err == EPERM || err == EACCES;
}
