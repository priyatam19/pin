# 0 "temp_no_pp.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "temp_no_pp.c"
# 30 "temp_no_pp.c"
  proper_name_lite ("Torbjorn Granlund", "Torbj\303\266rn Granlund"),
  proper_name ("Richard M. Stallman")


static char const *infile;


static int input_desc;




static char line_buf[LINE_COUNTER_BUF_LEN] =
  {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '0',
    '\t', '\0'
  };



static char *line_num_print = line_buf + LINE_COUNTER_BUF_LEN - 8;


static char *line_num_start = line_buf + LINE_COUNTER_BUF_LEN - 3;


static char *line_num_end = line_buf + LINE_COUNTER_BUF_LEN - 3;


static int newlines2 = 0;


static bool pending_cr = false;

void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    emit_try_help ();
  else
    {
      printf (_("Usage: %s [OPTION]... [FILE]...\n"),


              program_name);
      fputs (_("Concatenate FILE(s) to standard output.\n"),

    stdout);

      emit_stdin_note ();

      fputs (_("\n  -A, --show-all           equivalent to -vET\n  -b, --number-nonblank    number nonempty output lines, overrides -n\n  -e                       equivalent to -vE\n  -E, --show-ends          display $ at end of each line\n  -n, --number             number all output lines\n  -s, --squeeze-blank      suppress repeated empty output lines\n"),







    stdout);
      fputs (_("  -t                       equivalent to -vT\n  -T, --show-tabs          display TAB characters as ^I\n  -u                       (ignored)\n  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n"),




    stdout);
      fputs (HELP_OPTION_DESCRIPTION, stdout);
      fputs (VERSION_OPTION_DESCRIPTION, stdout);
      printf (_("\nExamples:\n  %s f - g  Output f's contents, then standard input, then g's contents.\n  %s        Copy standard input to standard output.\n"),





              program_name, program_name);
      emit_ancillary_info (PROGRAM_NAME);
    }
  exit (status);
}



static void
next_line_num (void)
{
  char *endp = line_num_end;
  do
    {
      if ((*endp)++ < '9')
        return;
      *endp-- = '0';
    }
  while (endp >= line_num_start);

  if (line_num_start > line_buf)
    *--line_num_start = '1';
  else
    *line_buf = '>';
  if (line_num_start < line_num_print)
    line_num_print--;
}





static bool
simple_cat (char *buf, idx_t bufsize)
{


  while (true)
    {


      ptrdiff_t n_read = safe_read (input_desc, buf, bufsize);
      if (n_read < 0)
        {
          error (0, errno, "%s", quotef (infile));
          return false;
        }



      if (n_read == 0)
        return true;



      if (full_write (STDOUT_FILENO, buf, n_read) != n_read)
        write_error ();
    }
}





static inline void
write_pending (char *outbuf, char **bpout)
{
  idx_t n_write = *bpout - outbuf;
  if (0 < n_write)
    {
      if (full_write (STDOUT_FILENO, outbuf, n_write) != n_write)
        write_error ();
      *bpout = outbuf;
    }
}
# 193 "temp_no_pp.c"
static bool
cat (char *inbuf, idx_t insize, char *outbuf, idx_t outsize,
     bool show_nonprinting, bool show_tabs, bool number, bool number_nonblank,
     bool show_ends, bool squeeze_blank)
{

  unsigned char ch;






  int newlines = newlines2;



  bool use_fionread = true;






  char *eob = inbuf;


  char *bpin = eob + 1;


  char *bpout = outbuf;

  while (true)
    {
      do
        {


          if (outbuf + outsize <= bpout)
            {
              char *wp = outbuf;
              idx_t remaining_bytes;
              do
                {
                  if (full_write (STDOUT_FILENO, wp, outsize) != outsize)
                    write_error ();
                  wp += outsize;
                  remaining_bytes = bpout - wp;
                }
              while (outsize <= remaining_bytes);




              memmove (outbuf, wp, remaining_bytes);
              bpout = outbuf + remaining_bytes;
            }



          if (bpin > eob)
            {
              bool input_pending = false;
              int n_to_read = 0;





              if (use_fionread
                  && ioctl (input_desc, FIONREAD, &n_to_read) < 0)
                {






                  if (errno == EOPNOTSUPP || errno == ENOTTY
                      || errno == EINVAL || errno == ENODEV
                      || errno == ENOSYS)
                    use_fionread = false;
                  else
                    {
                      error (0, errno, _("cannot do ioctl on %s"),
                             quoteaf (infile));
                      newlines2 = newlines;
                      return false;
                    }
                }
              if (n_to_read != 0)
                input_pending = true;

              if (!input_pending)
                write_pending (outbuf, &bpout);



              ptrdiff_t n_read = safe_read (input_desc, inbuf, insize);
              if (n_read < 0)
                {
                  error (0, errno, "%s", quotef (infile));
                  write_pending (outbuf, &bpout);
                  newlines2 = newlines;
                  return false;
                }
              if (n_read == 0)
                {
                  write_pending (outbuf, &bpout);
                  newlines2 = newlines;
                  return true;
                }




              bpin = inbuf;
              eob = bpin + n_read;
              *eob = '\n';
            }
          else
            {





              if (++newlines > 0)
                {
                  if (newlines >= 2)
                    {



                      newlines = 2;




                      if (squeeze_blank)
                        {
                          ch = *bpin++;
                          continue;
                        }
                    }



                  if (number && !number_nonblank)
                    {
                      next_line_num ();
                      bpout = stpcpy (bpout, line_num_print);
                    }
                }


              if (show_ends)
                {
                  if (pending_cr)
                    {
                      *bpout++ = '^';
                      *bpout++ = 'M';
                      pending_cr = false;
                    }
                  *bpout++ = '$';
                }



              *bpout++ = '\n';
            }
          ch = *bpin++;
        }
      while (ch == '\n');



      if (pending_cr)
        {
          *bpout++ = '\r';
          pending_cr = false;
        }



      if (newlines >= 0 && number)
        {
          next_line_num ();
          bpout = stpcpy (bpout, line_num_print);
        }







      if (show_nonprinting)
        {
          while (true)
            {
              if (ch >= 32)
                {
                  if (ch < 127)
                    *bpout++ = ch;
                  else if (ch == 127)
                    {
                      *bpout++ = '^';
                      *bpout++ = '?';
                    }
                  else
                    {
                      *bpout++ = 'M';
                      *bpout++ = '-';
                      if (ch >= 128 + 32)
                        {
                          if (ch < 128 + 127)
                            *bpout++ = ch - 128;
                          else
                            {
                              *bpout++ = '^';
                              *bpout++ = '?';
                            }
                        }
                      else
                        {
                          *bpout++ = '^';
                          *bpout++ = ch - 128 + 64;
                        }
                    }
                }
              else if (ch == '\t' && !show_tabs)
                *bpout++ = '\t';
              else if (ch == '\n')
                {
                  newlines = -1;
                  break;
                }
              else
                {
                  *bpout++ = '^';
                  *bpout++ = ch + 64;
                }

              ch = *bpin++;
            }
        }
      else
        {

          while (true)
            {
              if (ch == '\t' && show_tabs)
                {
                  *bpout++ = '^';
                  *bpout++ = ch + 64;
                }
              else if (ch != '\n')
                {
                  if (ch == '\r' && *bpin == '\n' && show_ends)
                    {
                      if (bpin == eob)
                        pending_cr = true;
                      else
                        {
                          *bpout++ = '^';
                          *bpout++ = 'M';
                        }
                    }
                  else
                    *bpout++ = ch;
                }
              else
                {
                  newlines = -1;
                  break;
                }

              ch = *bpin++;
            }
        }
    }
}





static int
copy_cat (void)
{



  ssize_t copy_max = MIN (SSIZE_MAX, SIZE_MAX) >> 30 << 30;







  for (bool some_copied = false; ; some_copied = true)
    switch (copy_file_range (input_desc, nullptr, STDOUT_FILENO, nullptr,
                             copy_max, 0))
      {
      case 0:
        return some_copied;

      case -1:
        if (errno == ENOSYS || is_ENOTSUP (errno) || errno == EINVAL
            || errno == EBADF || errno == EXDEV || errno == ETXTBSY
            || errno == EPERM)
          return 0;
        error (0, errno, "%s", quotef (infile));
        return -1;
      }
}


int
main (int argc, char **argv)
{

  bool have_read_stdin = false;

  struct stat stat_buf;


  bool number = false;
  bool number_nonblank = false;
  bool squeeze_blank = false;
  bool show_ends = false;
  bool show_nonprinting = false;
  bool show_tabs = false;
  int file_open_mode = O_RDONLY;

  static struct option const long_options[] =
  {
    {"number-nonblank", no_argument, nullptr, 'b'},
    {"number", no_argument, nullptr, 'n'},
    {"squeeze-blank", no_argument, nullptr, 's'},
    {"show-nonprinting", no_argument, nullptr, 'v'},
    {"show-ends", no_argument, nullptr, 'E'},
    {"show-tabs", no_argument, nullptr, 'T'},
    {"show-all", no_argument, nullptr, 'A'},
    {GETOPT_HELP_OPTION_DECL},
    {GETOPT_VERSION_OPTION_DECL},
    {nullptr, 0, nullptr, 0}
  };

  initialize_main (&argc, &argv);
  set_program_name (argv[0]);
  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);





  atexit (close_stdout);



  int c;
  while ((c = getopt_long (argc, argv, "benstuvAET", long_options, nullptr))
         != -1)
    {
      switch (c)
        {
        case 'b':
          number = true;
          number_nonblank = true;
          break;

        case 'e':
          show_ends = true;
          show_nonprinting = true;
          break;

        case 'n':
          number = true;
          break;

        case 's':
          squeeze_blank = true;
          break;

        case 't':
          show_tabs = true;
          show_nonprinting = true;
          break;

        case 'u':

          break;

        case 'v':
          show_nonprinting = true;
          break;

        case 'A':
          show_nonprinting = true;
          show_ends = true;
          show_tabs = true;
          break;

        case 'E':
          show_ends = true;
          break;

        case 'T':
          show_tabs = true;
          break;

        case_GETOPT_HELP_CHAR;

        case_GETOPT_VERSION_CHAR (PROGRAM_NAME, AUTHORS);

        default:
          usage (EXIT_FAILURE);
        }
    }



  if (fstat (STDOUT_FILENO, &stat_buf) < 0)
    error (EXIT_FAILURE, errno, _("standard output"));


  idx_t outsize = io_blksize (&stat_buf);


  dev_t out_dev = stat_buf.st_dev;
  ino_t out_ino = stat_buf.st_ino;
  int out_flags = -2;


  bool out_isreg = S_ISREG (stat_buf.st_mode) != 0;

  if (! (number || show_ends || squeeze_blank))
    {
      file_open_mode |= O_BINARY;
      xset_binary_mode (STDOUT_FILENO, O_BINARY);
    }



  infile = "-";
  int argind = optind;
  bool ok = true;
  idx_t page_size = getpagesize ();

  do
    {
      if (argind < argc)
        infile = argv[argind];

      bool reading_stdin = STREQ (infile, "-");
      if (reading_stdin)
        {
          have_read_stdin = true;
          input_desc = STDIN_FILENO;
          if (file_open_mode & O_BINARY)
            xset_binary_mode (STDIN_FILENO, O_BINARY);
        }
      else
        {
          input_desc = open (infile, file_open_mode);
          if (input_desc < 0)
            {
              error (0, errno, "%s", quotef (infile));
              ok = false;
              continue;
            }
        }

      if (fstat (input_desc, &stat_buf) < 0)
        {
          error (0, errno, "%s", quotef (infile));
          ok = false;
          goto contin;
        }


      idx_t insize = io_blksize (&stat_buf);

      fdadvise (input_desc, 0, 0, FADVISE_SEQUENTIAL);





      if (stat_buf.st_dev == out_dev && stat_buf.st_ino == out_ino)
        {
          if (out_flags < -1)
            out_flags = fcntl (STDOUT_FILENO, F_GETFL);
          bool exhausting = 0 <= out_flags && out_flags & O_APPEND;
          if (!exhausting)
            {
              off_t in_pos = lseek (input_desc, 0, SEEK_CUR);
              if (0 <= in_pos)
                exhausting = in_pos < lseek (STDOUT_FILENO, 0, SEEK_CUR);
            }
          if (exhausting)
            {
              error (0, 0, _("%s: input file is output file"), quotef (infile));
              ok = false;
              goto contin;
            }
        }


      char *inbuf;





      if (! (number || show_ends || show_nonprinting
             || show_tabs || squeeze_blank))
        {
          int copy_cat_status =
            out_isreg && S_ISREG (stat_buf.st_mode) ? copy_cat () : 0;
          if (copy_cat_status != 0)
            {
              inbuf = nullptr;
              ok &= 0 < copy_cat_status;
            }
          else
            {
              insize = MAX (insize, outsize);
              inbuf = xalignalloc (page_size, insize);
              ok &= simple_cat (inbuf, insize);
            }
        }
      else
        {

          inbuf = xalignalloc (page_size, insize + 1);
# 756 "temp_no_pp.c"
          idx_t bufsize;
          if (ckd_mul (&bufsize, insize, 4)
              || ckd_add (&bufsize, bufsize, outsize)
              || ckd_add (&bufsize, bufsize, LINE_COUNTER_BUF_LEN - 1))
            xalloc_die ();
          char *outbuf = xalignalloc (page_size, bufsize);

          ok &= cat (inbuf, insize, outbuf, outsize, show_nonprinting,
                     show_tabs, number, number_nonblank, show_ends,
                     squeeze_blank);

          alignfree (outbuf);
        }

      alignfree (inbuf);

    contin:
      if (!reading_stdin && close (input_desc) < 0)
        {
          error (0, errno, "%s", quotef (infile));
          ok = false;
        }
    }
  while (++argind < argc);

  if (pending_cr)
    {
      if (full_write (STDOUT_FILENO, "\r", 1) != 1)
        write_error ();
    }

  if (have_read_stdin && close (STDIN_FILENO) < 0)
    error (EXIT_FAILURE, errno, _("closing standard input"));

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
