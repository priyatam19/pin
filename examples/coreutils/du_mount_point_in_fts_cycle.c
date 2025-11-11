/* Extracted from GNU coreutils du.c */
/* Function: mount_point_in_fts_cycle */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool
mount_point_in_fts_cycle (FTSENT const *ent){
  FTSENT const *cycle_ent = ent->fts_cycle;

  if (!di_mnt)
    {
      /* Initialize the set of dev,inode pairs.  */
      di_mnt = di_set_alloc ();
      if (!di_mnt)
        xalloc_die ();

      fill_mount_table ();
    }

  while (ent && ent != cycle_ent)
    {
      if (di_set_lookup (di_mnt, ent->fts_statp->st_dev,
                         ent->fts_statp->st_ino) > 0)
        {
          return true;
        }
      ent = ent->fts_parent;
    }

  return false;
}
