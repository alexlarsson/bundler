#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <fcntl.h>

#define MIN(_x, _y) ((_x < _y) ? _x : _y)

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

static ssize_t
do_pread (int fd, void *_buf, size_t size, off_t offset)
{
  char *buf = _buf;
  size_t read = 0;
  ssize_t res;

  while (size > 0)
    {
      res = pread (fd, buf, size, offset + read);

      if (res < 0)
	{
	  if (errno != EINTR)
	    return res;
	}
      else if (res == 0)
	return -1;
      else
	{
	  read += res;
	  size -= res;
	}
    }

  return read;
}

static void
get_section32_by_name (int fd,
		       Elf32_Ehdr *ehdr,
		       char *section,
		       Elf32_Shdr *shdr)
{
  char *shdrs;
  Elf32_Shdr *s;
  Elf32_Shdr *shstrtable;
  char *strtab;
  int i;

  shdrs = xmalloc (ehdr->e_shentsize * ehdr->e_shnum);
  if (do_pread (fd, shdrs, ehdr->e_shentsize * ehdr->e_shnum,
		ehdr->e_shoff) < 0)
    die ("Can't read headers");

  shstrtable = (Elf32_Shdr *)
    (shdrs + ehdr->e_shentsize * ehdr->e_shstrndx);

  strtab = xmalloc (shstrtable->sh_size + 1);
  if (do_pread (fd, strtab, shstrtable->sh_size,
		shstrtable->sh_offset) < 0)
    die ("Can't read shstrtable");
  /* Ensure null termination */
  strtab[shstrtable->sh_size] = 0;

  for (i = 0; i < ehdr->e_shnum; i++)
    {
      s = (Elf32_Shdr *)
	(shdrs + ehdr->e_shentsize * i);
      if (s->sh_name < shstrtable->sh_size &&
	  strcmp (strtab + s->sh_name, section) == 0)
	{
	  *shdr = *s;
	  break;
	}
    }

  if (i == ehdr->e_shnum)
    die ("No section %s\n", section);

  free (strtab);
  free (shdrs);
}

static void
get_section64_by_name (int fd,
		       Elf64_Ehdr *ehdr,
		       char *section,
		       Elf64_Shdr *shdr)
{
  char *shdrs;
  Elf64_Shdr *s;
  Elf64_Shdr *shstrtable;
  char *strtab;
  int i;

  shdrs = xmalloc (ehdr->e_shentsize * ehdr->e_shnum);
  if (do_pread (fd, shdrs, ehdr->e_shentsize * ehdr->e_shnum,
		ehdr->e_shoff) < 0)
    die ("Can't read headers");

  shstrtable = (Elf64_Shdr *)
    (shdrs + ehdr->e_shentsize * ehdr->e_shstrndx);

  strtab = xmalloc (shstrtable->sh_size + 1);
  if (do_pread (fd, strtab, shstrtable->sh_size,
		shstrtable->sh_offset) < 0)
    die ("Can't read shstrtable");
  /* Ensure null termination */
  strtab[shstrtable->sh_size] = 0;

  for (i = 0; i < ehdr->e_shnum; i++)
    {
      s = (Elf64_Shdr *)
	(shdrs + ehdr->e_shentsize * i);
      if (s->sh_name < shstrtable->sh_size &&
	  strcmp (strtab + s->sh_name, section) == 0)
	{
	  *shdr = *s;
	  break;
	}
    }

  if (i == ehdr->e_shnum)
    die ("No section %s\n", section);

  free (strtab);
  free (shdrs);
}

static Elf64_Off
get_elf_section_offset (int fd, char *section)
{
  int class;
  union {
    Elf32_Ehdr hdr32;
    Elf64_Ehdr hdr64;
  } hdr;

  if (do_pread (fd, &hdr, sizeof (hdr), 0) < 0)
    die ("Unable to open executable\n");

  if (memcmp (&hdr, ELFMAG, SELFMAG) != 0)
    die ("Invalid ELF header\n");

  class = hdr.hdr32.e_ident[EI_CLASS];
  if (class != ELFCLASS32 && class != ELFCLASS64)
    die ("Invalid ELF class\n");

  if (class == ELFCLASS32)
    {
      Elf32_Shdr shdr;
      get_section32_by_name (fd, &hdr.hdr32,
			     section,
			     &shdr);
      return shdr.sh_offset;
    }
  else
    {
      Elf64_Shdr shdr;
      get_section64_by_name (fd, &hdr.hdr64,
			     section,
			     &shdr);
      return shdr.sh_offset;
    }
}

int
main (int argc, char **argv)
{
  int fd, i, j;
  Elf64_Off offset;
  char fd_buf[128];
  char offset_buf[128];
  char **child_argv;

  fd = open ("/proc/self/exe", O_RDONLY);

  if (fd < 0)
    {
      fprintf (stderr, "Unable to open executable\n");
      return 1;
    }

  offset = get_elf_section_offset (fd, ".bundle");
  snprintf (fd_buf, sizeof(fd_buf), "%d", fd);
  snprintf (offset_buf, sizeof(offset_buf), "%ld", (long) offset);

  child_argv = xmalloc ((2 + argc + 1) * sizeof (char *));
  j = 0;
  child_argv[j++] = argv[0];
  child_argv[j++] = fd_buf;
  child_argv[j++] = offset_buf;
  for (i = 1; i < argc; i++)
    child_argv[j++] = argv[i];
  child_argv[j++] = NULL;

  return execv ("./helper", child_argv);
}
