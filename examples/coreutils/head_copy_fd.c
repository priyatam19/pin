/* Extracted from GNU coreutils head.c */
/* Function: copy_fd */

#include <stdio.h>

enum Copy_fd_status
copy_fd (int src_fd, uintmax_t n_bytes){
  char buf[BUFSIZ];

  /* Copy the file contents.  */
  while (0 < n_bytes)
    {
      idx_t n_to_read = MIN (n_bytes, sizeof buf);
      ptrdiff_t n_read = safe_read (src_fd, buf, n_to_read);
      if (n_read < 0)
        return COPY_FD_READ_ERROR;

      n_bytes -= n_read;

      if (n_read == 0 && n_bytes != 0)
        return COPY_FD_UNEXPECTED_EOF;

      xwrite_stdout (buf, n_read);
    }

  return COPY_FD_OK;
}
