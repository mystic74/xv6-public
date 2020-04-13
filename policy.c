#include "types.h"
#include "stat.h"
#include "user.h"
//#include <stdlib.h>

int main(int argc, char *argv[])
{
    int rVal;
    if (argc != 2)
        exit(1);
    printf(1, "Changing policy to %s\n", argv[1]);
    rVal = set_policy(atoi(argv[1]));
    if (rVal == 0)
    {
        printf(1, "%s", "Policy has been successfully changed to");
    }
    else
    {
        printf(1, "Error replacing policy, no such a policy number %s\n", argv[1]);
    }

    exit(0);
}
