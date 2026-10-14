#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

// Same order as enum procstate in kernel/proc.h
static char *states[] = {"unused", "used", "sleeping", "runnable", "running", "zombie"};

int
main(void)
{
  static struct uproc procs[NPROC];   // static: 64 entries is too big for the 4 KB user stack
  int n = getprocs(procs, NPROC);

  if (n < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }
  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");
  for (int i = 0; i < n; i++) {
    char *st = (procs[i].state >= 0 && procs[i].state < 6) ?
               states[procs[i].state] : "???";
    printf("%d\t%d\t%s\t%lu\t%s\n", procs[i].pid, procs[i].ppid, st,
           procs[i].sz, procs[i].name);
  }
  exit(0);
}
