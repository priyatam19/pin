/* Extracted from GNU coreutils dd.c */
/* Function: advance_input_after_read_error */

#include <ctype.h>

bool
advance_input_after_read_error (idx_t nbytes){
  if (! input_seekable)
    {
      if (input_seek_errno == ESPIPE)
        return true;
      errno = input_seek_errno;
    }
  else
    {
      off_t offset;
      advance_input_offset (nbytes);
      if (input_offset < 0)
        {
          diagnose (0, _("offset overflow while reading file %s"),
                    quoteaf (input_file));
          return false;
        }
      offset = lseek (STDIN_FILENO, 0, SEEK_CUR);
      if (0 <= offset)
        {
          off_t diff;
          if (offset == input_offset)
            return true;
          diff = input_offset - offset;
          if (! (0 <= diff && diff <= nbytes) && status_level != STATUS_NONE)
            diagnose (0, _("warning: invalid file offset after failed read"));
          if (0 <= lseek (STDIN_FILENO, diff, SEEK_CUR))
            return true;
          if (errno == 0)
            diagnose (0, _("cannot work around kernel bug after all"));
        }
    }

  diagnose (errno, _("%s: cannot seek"), quotef (input_file));
  return false;
}
