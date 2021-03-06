#include "types.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "user.h"

void dummy_loop()
{
    struct perf *performance = (struct perf *)malloc(sizeof(struct perf));
    volatile int i = 100000000;
    int uptime_digit = uptime();
    uptime_digit = uptime_digit % 100;
    int dummy = 0;
    while (i--)
        dummy += i;

    sleep(uptime_digit);
    proc_info(performance);

    printf(1, "PID  PS_PRIORITY  STIME  RETIME  RTIME \n");
    printf(1, "%d    %d            %d     %d      %d\n",
           getpid(), performance->ps_priority, performance->stime, performance->retime, performance->rtime);
    free(performance);
    exit(0);
}

int main()
{
    int pid1, pid2, pid3;
    if ((pid1 = fork()) == 0)
    {
        set_cfs_priority(3);
        set_ps_priority(10);
        dummy_loop();
    }
    if ((pid2 = fork()) == 0)
    {
        set_cfs_priority(2);
        set_ps_priority(5);
        dummy_loop();
    }
    if ((pid3 = fork()) == 0)
    {
        set_cfs_priority(1);
        set_ps_priority(1);
        dummy_loop();
    }
    while ((wait(0)) > 0)
        ;

    exit(0);
}