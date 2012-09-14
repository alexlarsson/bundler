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
#include <stdarg.h>

#if 0
#define __debug__(x) printf x
#else
#define __debug__(x)
#endif

#ifndef MS_PRIVATE      /* May not be defined in older glibc headers */
#define MS_PRIVATE (1<<18) /* change to private */
#endif

static void
die (const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  exit (1);
}

static void *
xmalloc (size_t size)
{
  void *res = malloc (size);
  if (res == NULL)
    die ("oom");
  return res;
}

int
main (int argc,
      char **argv)
{
  int res;
  char *source;
  char *executable;
  char **child_argv;
  int i, j, argv_offset;
  int mount_count;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: bundler-builder <dir> [<command> [args]]");
      return 1;
    }

  /* The initial code is run with a high permission euid
     (at least CAP_SYS_ADMIN), so take lots of care. */

  __debug__(("Opening bundle\n"));

  source = argv[1];
  if (argc >= 3)
    {
      executable = argv[2]; 
      argv_offset = 3;
    }
  else
    {
      executable = "/bin/sh";
      argv_offset = 2;
    }

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

  __debug__(("mount source %s to %s\n", source, BUNDLE_PREFIX));
  res = mount (source, BUNDLE_PREFIX,
	       NULL, MS_BIND, NULL);
  if (res != 0)
    {
      perror ("Failed to mount the source device");
      goto error_out;
    }
  mount_count++; /* Normal mount succeeded */

  /* Now we have everything we need CAP_SYS_ADMIN for, so drop setuid */
  setuid (getuid ());

  child_argv = NULL;

  child_argv = xmalloc ((1 + argc - argv_offset + 1) * sizeof (char *));

  j = 0;
  child_argv[j++] = executable;
  for (i = argv_offset; i < argc; i++)
    child_argv[j++] = argv[i];
  child_argv[j++] = NULL;

  __debug__(("launch executable %s\n", executable));
  return execvp (executable, child_argv);

 error_out:
  while (mount_count-- > 0)
    umount (BUNDLE_PREFIX);
  return 1;
}
