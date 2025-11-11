/* Extracted from GNU coreutils dd.c */
/* Function: invalidate_cache */

#include <ctype.h>

bool
invalidate_cache (int fd, off_t len){
  int adv_ret = -1;
  off_t offset;
  bool nocache_eof = (fd == STDIN_FILENO ? i_nocache_eof : o_nocache_eof);

  /* Minimize syscalls.  */
  off_t clen = cache_round (fd, len);
  if (len && !clen)
    return true; /* Don't advise this time.  */
  else if (! len && ! clen && ! nocache_eof)
    return true;
  off_t pending = len ? cache_round (fd, 0) : 0;

  if (fd == STDIN_FILENO)
    {
      if (input_seekable)
        offset = input_offset;
      else
        {
          offset = -1;
          errno = ESPIPE;
        }
    }
  else
    {
      static off_t output_offset = -2;

      if (output_offset != -1)
        {
          if (output_offset < 0)
            output_offset = lseek (fd, 0, SEEK_CUR);
          else if (len)
            output_offset += clen + pending;
        }

      offset = output_offset;
    }

  if (0 <= offset)
   {
     if (! len && clen && nocache_eof)
       {
         pending = clen;
         clen = 0;
       }

     /* Note we're being careful here to only invalidate what
        we've read, so as not to dump any read ahead cache.
        Note also the kernel is conservative and only invalidates
        full pages in the specified range.  */
#if HAVE_POSIX_FADVISE
     offset = offset - clen - pending;
     /* ensure full page specified when invalidating to eof.  */
     if (clen == 0)
       offset -= offset % page_size;
     adv_ret = posix_fadvise (fd, offset, clen, POSIX_FADV_DONTNEED);
#else
     errno = ENOTSUP;
#endif
   }

  return adv_ret != -1 ? true : false;
}
