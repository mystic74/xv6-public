#include "types.h"
#include "param.h"
#include "mmu.h"
#include "user.h"

void dummy_loop()
{
    volatile int i = 0xDEADBABE;
    int uptime_org = uptime();
    int uptime_digit = uptime_org % 100;
    
    int dummy = 0;
    while (i--)
        dummy += i;

    sleep(uptime_digit);

    printf(1, "%d was here, slept for %x seconds, lived for %d ticks and died :( \n",
                getpid(), uptime_digit, uptime()- uptime_org);
    exit();
}

int main()
{
    int pid1, pid2, pid3;

    if ((pid1 = fork()) == 0)
    {
        dummy_loop();
    }
    sleep(uptime()%20);
    if ((pid2 = fork()) == 0)
    {
        dummy_loop();
    }
    sleep(uptime()%20);
    if ((pid3 = fork()) == 0)
    {
        dummy_loop();
    }
    
    while ((wait()) > 0)
        ;

    exit();
}