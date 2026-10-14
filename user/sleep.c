#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc != 2 || argv[1][0] == 0)
    goto usage;
  for (char *s = argv[1]; *s; s++)
    if (*s < '0' || *s > '9')
      goto usage;
  pause(atoi(argv[1]));
  exit(0);

usage:
  fprintf(2, "usage: sleep <ticks>\n");
  exit(1);
}
