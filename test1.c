#include "types.h"
#include "param.h"
#include "mmu.h"
#include "user.h"


#define proc_number 3
#define VERBOSE 1
void stupid_handler1(int numnum);
void stupid_handler2(int numnum);
void dummy_loop();


int pid_array[proc_number+1];

inline void verbose_log(int loglevel, const char* printval)
{
    if (VERBOSE > loglevel)
    {
        printf(1, "%s", printval);
    }
}

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

void dummy_action()
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0x0;
    
    printf(1," address is %x, setting in param \n", &stupid_handler1);
    printf(1, "This is the action registering function for proc :%d  \n", getpid());

    sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);


}

void test_inherit_mask()
{
    int childid;
    childid = fork();
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = (1 << (MY_SIGSIG);

    sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);

    if (childid == 0)
    {
        // In child we should halt for a while.
        dummy_action();

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, MY_SIGSIG);
    }
}

void test_mask()
{
    int childid;
    childid = fork();
    struct sigaction mystruct;
    
    
    if (childid == 0)
    {
        mystruct.sa_handler = &stupid_handler2;
        mystruct.sigmask = (1 << (MY_SIGSIG);

        sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(3);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, MY_SIGSIG);
    }
}

void test_kill_suspended()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        dummy_action();

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);

        kill(childid, SIGKILL);
    }    
}

void test_handler_suspended()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        dummy_action();

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);

        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);

        // Sending Continue
        kill (childid, SIGCONT);
    }
    
}


void test_handler_suspended_no_cont()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        dummy_action();

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);

        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);
    }
    
}


void stupid_handler1(int numnum)
{
    printf(1, "this is the stupid signal handler! \n");
    printf(1, "this is the signal number! %d \n", numnum);
}


void stupid_handler2(int numnum)
{
    printf(1, "this is the another! signal handler! \n");
    printf(1, "2222222222222222222222222222222222222 \n");
    printf(1, "this is the signal number! %d \n", numnum);
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
        
        kill (send_to - 1,MY_SIGSIG);
        kill (send_to,MY_SIGSIG);
    }
}


void test_basic_bitch()
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
        
        //sending_kernel_signals();
        dummy_loop();
        //dummy_loop();
    }
}

int main()
{
    test_basic_bitch();

    while ((wait()) > 0)
        ;

    exit();
}

