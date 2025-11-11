/* Extracted from GNU coreutils iopoll.c */
/* Function: iopoll_internal */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int
iopoll_internal (int fdin, int fdout, bool block, bool broken_output){
  affirm (fdin != -1 || fdout != -1);

#if IOPOLL_USES_POLL
  struct pollfd pfds[2] = {  /* POLLRDBAND needed for illumos, macOS.  */
    { .fd = fdin,  .events = POLLIN | POLLRDBAND, .revents = 0 },
    { .fd = fdout, .events = POLLRDBAND, .revents = 0 },
  };
  int check_out_events = POLLERR | POLLHUP | POLLNVAL;
  int ret = 0;

  if (! broken_output)
    {
      pfds[0].events = pfds[1].events = POLLOUT;
      check_out_events = POLLOUT;
    }

  while (0 <= ret || errno == EINTR)
    {
      ret = poll (pfds, 2, block ? -1 : 0);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      affirm (0 < ret);
      if (pfds[0].revents) /* input available or pipe closed indicating EOF; */
        return 0;          /* should now be able to read() without blocking  */
      if (pfds[1].revents & check_out_events)
        return broken_output ? IOPOLL_BROKEN_OUTPUT : 0;
    }

#else  /* fall back to select()-based implementation */

  int nfds = (fdin > fdout ? fdin : fdout) + 1;
  int ret = 0;

  if (FD_SETSIZE < nfds)
    {
      errno = EINVAL;
      ret = -1;
    }

  /* If fdout has an error condition (like a broken pipe) it will be seen
     as ready for reading.  Assumes fdout is not actually readable.  */
  while (0 <= ret || errno == EINTR)
    {
      fd_set fds;
      FD_ZERO (&fds);
      if (0 <= fdin)
        FD_SET (fdin, &fds);
      if (0 <= fdout)
        FD_SET (fdout, &fds);

      struct timeval delay = {0};
      ret = select (nfds,
                    broken_output ? &fds : nullptr,
                    broken_output ? nullptr : &fds,
                    nullptr, block ? nullptr : &delay);

      if (ret < 0)
        continue;
      if (ret == 0 && ! block)
        return 0;
      affirm (0 < ret);
      if (0 <= fdin && FD_ISSET (fdin, &fds))    /* input available or EOF; */
        return 0;          /* should now be able to read() without blocking */
      if (0 <= fdout && FD_ISSET (fdout, &fds))  /* equiv to POLLERR        */
        return broken_output ? IOPOLL_BROKEN_OUTPUT : 0;
    }

#endif
  return IOPOLL_ERROR;
}
