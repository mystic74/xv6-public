#include "types.h"
#include "param.h"
#include "mmu.h"
#include "user.h"


#define proc_number 3


int pid_array[proc_number+1];
void init_pid_array()
{
    int i;
    for (i=0;i<proc_number+1;i++)
    {
        pid_array[i] = -1;
    }

}
void dummy_sleep()
{
    int uptime_org = uptime();
    int uptime_digit = uptime_org % 100;
    sleep(uptime_digit);
}
#define MY_SIGSIG 11

void stupid_handler1(int numnum)
{
    printf(1, "this is the stupid signal handler! \n");
    printf(1, "this is the signal number! %d \n", numnum);
}


void dummy_action()
{
    struct sigaction mystruct;
    mystruct.sa_handler = stupid_handler1;
    mystruct.sigmask = 0;
    
    printf(1, "This is the action registering function \n");

    sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);


}
void dummy_loop()
{
    volatile int i = 0xDEADBABE;
    int uptime_org = uptime();
    int uptime_digit = uptime_org % 100;
    
        dummy_action();
   
    int dummy = 0;
    while (i--)
        dummy += i;

    sleep(uptime_digit);

    printf(1, "%d was here, slept for %x seconds, lived for %d ticks and died :( \n",
                getpid(), uptime_digit, uptime()- uptime_org);
    exit();
}

int find_pid (int curpid)
{
    int i;
    for (i=1;i<proc_number+1;i++)
    {
        if (pid_array[i]!=curpid && pid_array[i] != -1)
            return pid_array[i];
    }
    return -1;
}

void sending_kernel_signals()
{
    
    int curpid = getpid();
    int send_to = find_pid(curpid);
    if (send_to != -1)
    {
        
        kill (send_to, SIGKILL);
        
        kill (send_to,SIGSTOP);
        
        kill (send_to,SIGCONT);
        
        kill (send_to,MY_SIGSIG);
    }
}


int main()
{
    init_pid_array();
    int pid1, pid2, pid3;

    if ((pid1 = fork()) == 0)
    {
        pid_array[1] = getpid();
        //dummy_sleep();

        dummy_loop();
    }
    sleep(uptime() % 20);
    if ((pid2 = fork()) == 0)
    {
        pid_array[2] = getpid();
        //dummy_sleep();
        dummy_loop();
    }
    sleep(uptime() % 20);
    if ((pid3 = fork()) == 0)
    {
        pid_array[3] = getpid();
        dummy_sleep();
        sending_kernel_signals();
        
        //dummy_loop();
    }
    
    while ((wait()) > 0)
        ;

    exit();
}

