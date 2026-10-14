#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

char *args[MAXARG + 1];
int nbase;                  // number of fixed args (cmd + its args)
int nargs;                  // current total in args[]
char words[MAXARG][128];    // storage for words read from stdin
int failed = 0;

void
usage(void)
{
  fprintf(2, "usage: xargs [-n N] cmd [args...]\n");
  exit(1);
}

void
run(void)
{
  int status;
  if (nargs == nbase)
    return;                 // nothing appended, nothing to run
  args[nargs] = 0;
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "xargs: exec %s failed\n", args[0]);
    exit(1);
  }
  wait(&status);
  if (status != 0)
    failed = 1;
  nargs = nbase;
}

int
main(int argc, char *argv[])
{
  int n = 0, i = 1;

  if (argc >= 2 && strcmp(argv[1], "-n") == 0) {
    if (argc < 4)
      usage();
    for (char *s = argv[2]; *s; s++)
      if (*s < '0' || *s > '9')
        usage();
    n = atoi(argv[2]);
    if (n < 1)
      usage();
    i = 3;
  }
  if (i >= argc)
    usage();

  nbase = 0;
  for (; i < argc; i++) {
    if (nbase >= MAXARG)
      usage();
    args[nbase++] = argv[i];
  }
  nargs = nbase;

  char c, word[128];
  int len = 0;
  while (1) {
    int r = read(0, &c, 1);
    int eof = (r != 1);
    int sep = eof || c == ' ' || c == '\t' || c == '\n';

    if (!sep) {
      if (len < sizeof(word) - 1)
        word[len++] = c;
      continue;
    }
    if (len > 0) {                 // finish the current word
      if (nargs >= MAXARG) {
        fprintf(2, "xargs: too many arguments\n");
        exit(1);
      }
      word[len] = 0;
      strcpy(words[nargs], word);
      args[nargs] = words[nargs];
      nargs++;
      len = 0;
      if (n > 0 && nargs - nbase == n)
        run();
    }
    if (eof)
      break;
    if (c == '\n' && n == 0)
      run();                       // one run per non-empty line
  }
  run();                           // leftover words
  exit(failed ? 1 : 0);
}
