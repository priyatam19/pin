/* Extracted from GNU coreutils csplit.c */
/* Function: write_to_file */

#include <ctype.h>

void
write_to_file (intmax_t last_line, bool ignore, int argnum){
  struct cstring *line;
  intmax_t first_line;		/* First available input line. */
  intmax_t lines;		/* Number of lines to output. */
  intmax_t i;

  first_line = get_first_line_in_buffer ();

  if (! first_line || first_line > last_line)
    {
      error (0, 0, _("%s: line number out of range"),
             quote (global_argv[argnum]));
      cleanup_fatal ();
    }

  lines = last_line - first_line;

  for (i = 0; i < lines; i++)
    {
      line = remove_line ();
      if (line == nullptr)
        {
          error (0, 0, _("%s: line number out of range"),
                 quote (global_argv[argnum]));
          cleanup_fatal ();
        }
      if (!ignore)
        save_line_to_file (line);
    }
}
