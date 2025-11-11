/* Extracted from GNU coreutils copy.c */
/* Function: copy_attr */

#include <stdio.h>

bool
copy_attr (char const *src_path, int src_fd,
           char const *dst_path, int dst_fd, struct cp_options const *x){
  bool all_errors = (!x->data_copy_required || x->require_preserve_xattr);
  bool some_errors = (!all_errors && !x->reduce_diagnostics);
  int (*check) (char const *, struct error_context *)
    = (x->preserve_security_context || x->set_security_context
       ? check_selinux_attr : nullptr);

# if 4 < __GNUC__ + (8 <= __GNUC_MINOR__)
  /* Pacify gcc -Wsuggest-attribute=format through at least GCC 13.2.1.  */
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
# endif
  struct error_context *ctx
    = (all_errors || some_errors
       ? (&(struct error_context) {
           .error = all_errors ? copy_attr_allerror : copy_attr_error,
           .quote = copy_attr_quote,
           .quote_free = copy_attr_free
         })
       : nullptr);
# if 4 < __GNUC__ + (8 <= __GNUC_MINOR__)
#  pragma GCC diagnostic pop
# endif

  return ! (0 <= src_fd && 0 <= dst_fd
            ? attr_copy_fd (src_path, src_fd, dst_path, dst_fd, check, ctx)
            : attr_copy_file (src_path, dst_path, check, ctx));
}
