/* Extracted from GNU coreutils copy.c */
/* Function: clone_file */

#include <stdio.h>

inline int
clone_file (int dest_fd, int src_fd){
#ifdef FICLONE
  return ioctl (dest_fd, FICLONE, src_fd);
#else
  (void) dest_fd;
  (void) src_fd;
  errno = ENOTSUP;
  return -1;
#endif
}
