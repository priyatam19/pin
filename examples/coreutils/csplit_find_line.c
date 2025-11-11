/* Extracted from GNU coreutils csplit.c */
/* Function: find_line */

#include <ctype.h>

struct cstring *
find_line (intmax_t linenum){
  struct buffer_record *b;

  if (head == nullptr && !load_buffer ())
    return nullptr;

  if (linenum < head->start_line)
    return nullptr;

  for (b = head;;)
    {
      if (linenum < b->start_line + b->num_lines)
        {
          /* The line is in this buffer. */
          struct line *l;
          idx_t offset;	/* How far into the buffer the line is. */

          l = b->line_start;
          offset = linenum - b->start_line;
          /* Find the control record. */
          while (offset >= CTRL_SIZE)
            {
              l = l->next;
              offset -= CTRL_SIZE;
            }
          return &l->starts[offset];
        }
      if (b->next == nullptr && !load_buffer ())
        return nullptr;
      b = b->next;		/* Try the next data block. */
    }
}
