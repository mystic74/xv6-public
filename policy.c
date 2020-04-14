#include "types.h"
#include "stat.h"
#include "user.h"
//#include <stdlib.h>

int main(int argc, char *argv[])
{
    char *polic_array[3]={"Default Policy", "Priority Policy", "CFS Policy"};
    int rVal;
    if (argc != 2)
        exit(1);
    int policy_num = atoi(argv[1]);
  
    rVal = set_policy(atoi(argv[1]));
    if (rVal == 0)
    {
        printf(1, "Policy has been successfully changed to %s\n", polic_array[policy_num]);
    }
    else
    {
        printf(1, "Error replacing policy, no such a policy number %s\n", argv[1]);
    }

    exit(0);
}
