#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  if((argc > 3) || (argc == 1)) {
    printf(2, "usage: kill <signal> pid...\n defualt signal is SIGKILL \n");
    exit();
  }
  if (argc == 2)
  {
    printf(1, "sending to %x \n", atoi(argv[1]));

    kill(atoi(argv[1]),SIGKILL);
  }
  else
  {
    printf(1, "sending %x \n", atoi(argv[1]));
    printf(1, "to %x \n", atoi(argv[2]));
    kill(atoi(argv[2]),atoi(argv[1]));
  } 
  exit();
}
