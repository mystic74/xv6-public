#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[])
{
    char *myArr;
    printf(1, "%s", "Entered memsizetest\n");
    printf(1, "%s%d \n", "The process is using:", mysize());
    myArr = malloc(2000);
    printf(1, "%s%d \n", "The process is using:", mysize());
    free(myArr);
    printf(1, "%s%d \n", "The process is using:", mysize());
    exit(0);
}
