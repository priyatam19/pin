/* Extracted from GNU coreutils copy.c */
/* Function: write_zeros */

#include <stdio.h>

bool
write_zeros (int fd, off_t n_bytes){
  static char *zeros;
  static size_t nz = IO_BUFSIZE;

  /* Attempt to use a relatively large calloc'd source buffer for
     efficiency, but if that allocation fails, resort to a smaller
     statically allocated one.  */
  if (zeros == nullptr)
    {
      static char fallback[1024];
      zeros = calloc (nz, 1);
      if (zeros == nullptr)
        {
          zeros = fallback;
          nz = sizeof fallback;
        }
    }

  while (n_bytes)
    {
      size_t n = MIN (nz, n_bytes);
      if ((full_write (fd, zeros, n)) != n)
        return false;
      n_bytes -= n;
    }

  return true;
}
