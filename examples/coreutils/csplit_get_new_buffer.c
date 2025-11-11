/* Extracted from GNU coreutils csplit.c */
/* Function: get_new_buffer */

#include <ctype.h>

struct buffer_record *
get_new_buffer (idx_t min_size){
  struct buffer_record *new_buffer = xmalloc (sizeof *new_buffer);
  new_buffer->bytes_alloc = 0;
  new_buffer->buffer = xpalloc (nullptr, &new_buffer->bytes_alloc, min_size,
                                -1, 1);
  new_buffer->bytes_used = 0;
  new_buffer->start_line = new_buffer->first_available = last_line_number + 1;
  new_buffer->num_lines = 0;
  new_buffer->line_start = new_buffer->curr_line = nullptr;
  new_buffer->next = nullptr;

  return new_buffer;
}
