/* Extracted from GNU coreutils df.c */
/* Function: filter_mount_list */

#include <stdio.h>

void
filter_mount_list (bool devices_only){
  struct mount_entry *me;

  /* Temporary list to keep entries ordered.  */
  struct devlist *device_list = nullptr;
  int mount_list_size = 0;

  for (me = mount_list; me; me = me->me_next)
    mount_list_size++;

  devlist_table = hash_initialize (mount_list_size, nullptr,
                                   devlist_hash, devlist_compare, nullptr);
  if (devlist_table == nullptr)
    xalloc_die ();

  /* Sort all 'wanted' entries into the list device_list.  */
  for (me = mount_list; me;)
    {
      struct stat buf;
      struct mount_entry *discard_me = nullptr;

      /* Avoid stating remote file systems as that may hang.
         On Linux we probably have me_dev populated from /proc/self/mountinfo,
         however we still stat() in case another device was mounted later.  */
      if ((me->me_remote && show_local_fs)
          || (me->me_dummy && !show_all_fs && !show_listed_fs)
          || (!selected_fstype (me->me_type) || excluded_fstype (me->me_type))
          || -1 == stat (me->me_mountdir, &buf))
        {
          /* If remote, and showing just local, or FS type is excluded,
             add ME for filtering later.
             If stat failed; add ME to be able to complain about it later.  */
          buf.st_dev = me->me_dev;
        }
      else
        {
          /* If we've already seen this device...  */
          struct devlist *seen_dev = devlist_for_dev (buf.st_dev);

          if (seen_dev)
            {
              bool target_nearer_root = strlen (seen_dev->me->me_mountdir)
                                        > strlen (me->me_mountdir);
              /* With bind mounts, prefer items nearer the root of the source */
              bool source_below_root = seen_dev->me->me_mntroot != nullptr
                                       && me->me_mntroot != nullptr
                                       && (strlen (seen_dev->me->me_mntroot)
                                           < strlen (me->me_mntroot));
              if (! print_grand_total
                  && me->me_remote && seen_dev->me->me_remote
                  && ! STREQ (seen_dev->me->me_devname, me->me_devname))
                {
                  /* Don't discard remote entries with different locations,
                     as these are more likely to be explicitly mounted.
                     However avoid this when producing a total to give
                     a more accurate value in that case.  */
                }
              else if ((strchr (me->me_devname, '/')
                       /* let "real" devices with '/' in the name win.  */
                        && ! strchr (seen_dev->me->me_devname, '/'))
                       /* let points towards the root of the device win.  */
                       || (target_nearer_root && ! source_below_root)
                       /* let an entry overmounted on a new device win...  */
                       || (! STREQ (seen_dev->me->me_devname, me->me_devname)
                           /* ... but only when matching an existing mnt point,
                              to avoid problematic replacement when given
                              inaccurate mount lists, seen with some chroot
                              environments for example.  */
                           && STREQ (me->me_mountdir,
                                     seen_dev->me->me_mountdir)))
                {
                  /* Discard mount entry for existing device.  */
                  discard_me = seen_dev->me;
                  seen_dev->me = me;
                }
              else
                {
                  /* Discard mount entry currently being processed.  */
                  discard_me = me;
                }

            }
        }

      if (discard_me)
        {
          me = me->me_next;
          if (! devices_only)
            free_mount_entry (discard_me);
        }
      else
        {
          /* Add the device number to the device_table.  */
          struct devlist *devlist = xmalloc (sizeof *devlist);
          devlist->me = me;
          devlist->dev_num = buf.st_dev;
          devlist->next = device_list;
          device_list = devlist;

          struct devlist *hash_entry = hash_insert (devlist_table, devlist);
          if (hash_entry == nullptr)
            xalloc_die ();
          /* Ensure lookups use this latest devlist.  */
          hash_entry->seen_last = devlist;

          me = me->me_next;
        }
    }

  /* Finally rebuild the mount_list from the devlist.  */
  if (! devices_only) {
    mount_list = nullptr;
    while (device_list)
      {
        /* Add the mount entry.  */
        me = device_list->me;
        me->me_next = mount_list;
        mount_list = me;
        struct devlist *next = device_list->next;
        free (device_list);
        device_list = next;
      }

      hash_free (devlist_table);
      devlist_table = nullptr;
  }
}
