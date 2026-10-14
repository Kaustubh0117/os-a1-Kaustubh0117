#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// 0 = no filter, 'f' = files only, 'd' = directories only
static int typefilter = 0;

// Returns 1 if string s matches pattern pat; '*' matches any sequence.
int
match(char *pat, char *s)
{
  if (*pat == 0)
    return *s == 0;
  if (*pat == '*')
    return match(pat + 1, s) || (*s && match(pat, s + 1));
  return *s == *pat && match(pat + 1, s + 1);
}

int
typeok(short type)
{
  if (typefilter == 'f')
    return type == T_FILE || type == T_DEVICE;
  if (typefilter == 'd')
    return type == T_DIR;
  return 1;
}

// The current path, plus the dirent and stat being examined. These are
// shared by every level of the recursion so that each level's stack frame
// stays small: the user stack is a single 4 KB page, and a local 512-byte
// buffer per level overflows it after only a few directories.
static char path[512];
static struct dirent de;
static struct stat st;

// Search the directory named by path (recursively). Each level appends
// "/<name>" to path while it examines an entry and removes it before returning.
void
find(char *pat, int top)
{
  int fd, len;

  if ((fd = open(path, O_RDONLY)) < 0) {
    if (top) {
      fprintf(2, "find: cannot open %s\n", path);
      exit(1);
    }
    return;
  }
  if (fstat(fd, &st) < 0 || st.type != T_DIR) {
    close(fd);
    if (top) {
      fprintf(2, "find: cannot open %s\n", path);
      exit(1);
    }
    return;
  }
  len = strlen(path);
  if (len + 1 + DIRSIZ + 1 > sizeof(path)) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }
  path[len] = '/';

  int nread = 0;                   // entries read so far, to resume after recursing
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    char *name = path + len + 1;
    nread++;
    if (de.inum == 0)
      continue;
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = 0;              // de.name may not be null-terminated
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;
    if (stat(path, &st) < 0)
      continue;
    if (match(pat, name) && typeok(st.type))
      printf("%s\n", path);
    if (st.type != T_DIR)
      continue;

    // Recurse whether or not the name matched. A process may only have
    // NOFILE (16) files open, so holding one per level would stop the
    // search about 13 levels down. Close this directory while recursing,
    // then reopen it and skip the entries already handled.
    close(fd);
    find(pat, 0);
    path[len] = 0;
    fd = open(path, O_RDONLY);
    if (fd < 0)
      return;
    path[len] = '/';
    for (int i = 0; i < nread; i++)
      if (read(fd, &de, sizeof(de)) != sizeof(de))
        break;
  }
  path[len] = 0;                   // strip "/<name>" again for the caller
  close(fd);
}

int
main(int argc, char *argv[])
{
  if (argc == 5 && strcmp(argv[3], "-type") == 0 &&
      (strcmp(argv[4], "f") == 0 || strcmp(argv[4], "d") == 0)) {
    typefilter = argv[4][0];
  } else if (argc != 3) {
    fprintf(2, "usage: find <dir> <pattern> [-type f|d]\n");
    exit(1);
  }
  if (strlen(argv[1]) >= sizeof(path)) {
    fprintf(2, "find: cannot open %s\n", argv[1]);
    exit(1);
  }
  strcpy(path, argv[1]);
  find(argv[2], 1);
  exit(0);
}
