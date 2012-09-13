#include "config.h"

#define _GNU_SOURCE /* Required for CLONE_NEWNS */
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/loop.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#if 0
#define __debug__(x) printf x
#else
#define __debug__(x)
#endif

#ifndef MS_PRIVATE      /* May not be defined in older glibc headers */
#define MS_PRIVATE (1<<18) /* change to private */
#endif

int
main (int argc,
      char **argv)
{
  int res;
  char *executable;
  char **child_argv;
  int i, j, fd, argv_offset;
  int mount_count;
  struct loop_info64 loopinfo;
  int loop_fd = -1;
  long offset;
  char loopname[128];

  if (argc < 3)
    {
      fprintf (stderr, "Too few arguments, need fd and offset");
      return 1;
    }

  /* The initial code is run with a high permission euid
     (at least CAP_SYS_ADMIN), so take lots of care. */

  __debug__(("Opening bundle\n"));

  fd = atoi (argv[1]);

  /* 0, 1, 2 are taken, also 0 generally means atoi error */
  if (fd < 3)
    {
      fprintf (stderr, "Invalid fd");
      return 1;
    }

  offset = atol (argv[2]);

  /* Find and set up loopback mount */
  __debug__(("Setting up loopback device\n"));
  for (i = 0 ; loop_fd < 0 ; i++ )
    {
      snprintf (loopname, sizeof (loopname), "/dev/loop%u", i);
      loop_fd = open (loopname, O_RDWR);
      if (loop_fd < 0 && errno == ENOENT)
	{
	  fprintf (stderr, "no available loopback device!");
	  return 1;
	}
      else if (loop_fd < 0)
	continue;

      if (ioctl (loop_fd, LOOP_SET_FD, (void *)(size_t)fd))
	{
	  close(loop_fd);
	  loop_fd = -1;
	  if (errno != EBUSY)
	    {
	      fprintf (stderr, "cannot set up loopback device %s", loopname);
	      return 1;
	    }
	  else
	    continue;
	}

      if (ioctl (loop_fd, LOOP_GET_STATUS64, &loopinfo))
	{
	  ioctl (loop_fd, LOOP_CLR_FD, 0);
	  fprintf (stderr, "cannot get up loopback device info");
	  return 1;
	}

      loopinfo.lo_offset = offset;
      loopinfo.lo_flags |= LO_FLAGS_READ_ONLY | LO_FLAGS_AUTOCLEAR;
      if (ioctl(loop_fd, LOOP_SET_STATUS64, &loopinfo))
	{
	  ioctl (loop_fd, LOOP_CLR_FD, 0);
	  fprintf (stderr, "cannot set up loopback device");
	  return 1;
	}
    }

  /* No need to keep fd open anymore */
  close (fd);

  __debug__(("creating new namespace\n"));
  res = unshare (CLONE_NEWNS);
  if (res != 0)
    {
      perror ("Creating new namespace failed");
      return 1;
    }

  /* Mark BUNDLE_PREFIX as a private mount to the
     new namespace. */
  __debug__(("mount bundle (private)\n"));
  mount_count = 0;
  res = mount (BUNDLE_PREFIX, BUNDLE_PREFIX,
	       NULL, MS_PRIVATE, NULL);
  if (res != 0 && errno == EINVAL) {
    /* Maybe if failed because there is no mount
       to be made private at that point, lets
       add a bind mount there. */
    __debug__(("mount bundle (bind)\n"));
    res = mount (BUNDLE_PREFIX, BUNDLE_PREFIX,
		 NULL, MS_BIND, NULL);
    /* And try again */
    if (res == 0)
      {
	mount_count++; /* Bind mount succeeded */
	__debug__(("mount bundle (private)\n"));
	res = mount (BUNDLE_PREFIX, BUNDLE_PREFIX,
		     NULL, MS_PRIVATE, NULL);
      }
  }

  if (res != 0)
    {
      perror ("Failed to make prefix namespace private");
      goto error_out;
    }

  __debug__(("mount source %s to %s\n", loopname, BUNDLE_PREFIX));
  res = mount (loopname, BUNDLE_PREFIX,
	       "squashfs", MS_MGC_VAL|MS_RDONLY, NULL);
  if (res != 0)
    {
      perror ("Failed to mount the loopback device");
      goto error_out;
    }
  mount_count++; /* Normal mount succeeded */

  /* No need to keep the loop device open after mount */
  close (loop_fd);
  loop_fd = -1;

  /* Now we have everything we need CAP_SYS_ADMIN for, so drop setuid */
  setuid (getuid ());

  executable = NULL;
  child_argv = NULL;

  executable = BUNDLE_PREFIX "/start";
  argv_offset = 3;

  child_argv = malloc ((1 + argc - argv_offset + 1) * sizeof (char *));
  if (child_argv == NULL)
    goto oom;

  j = 0;
  child_argv[j++] = argv[0];
  for (i = argv_offset; i < argc; i++)
    child_argv[j++] = argv[i];
  child_argv[j++] = NULL;

  __debug__(("launch executable %s\n", executable));
  return execv (executable, child_argv);

 oom:
  fprintf (stderr, "Out of memory.\n");

 error_out:
  if (loop_fd > 0)
    {
      ioctl (loop_fd, LOOP_CLR_FD, 0);
      close (loop_fd);
    }

  while (mount_count-- > 0)
    umount (BUNDLE_PREFIX);
  return 1;
}
