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

void
find(char *path, char *pat, int top)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

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
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
    fprintf(2, "find: path too long\n");
    close(fd);
    return;
  }
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;                 // de.name may not be null-terminated
    if (strcmp(p, ".") == 0 || strcmp(p, "..") == 0)
      continue;
    if (stat(buf, &st) < 0)
      continue;
    if (match(pat, p) && typeok(st.type))
      printf("%s\n", buf);
    if (st.type == T_DIR)
      find(buf, pat, 0);           // recurse whether or not the name matched
  }
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
  find(argv[1], argv[2], 1);
  exit(0);
}
