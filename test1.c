#include "types.h"
#include "param.h"
#include "mmu.h"
#include "user.h"

#define MY_SIGSIG 11
#define MY_OTHER_SIGIG 12
#define proc_number 3
#define VERBOSE 2
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

void reg_stupidhandler1_with_sig_and_mask(const int SIG, const int mask)
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = mask;
    
    sigaction(SIG,  &mystruct, (void*)NULL);
}


void reg_stupidhandler1_with_sig(const int SIG)
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0x0;
    
    sigaction(SIG,  &mystruct, (void*)NULL);
}


void reg_stupidhandler2_with_sig_and_mask(const int SIG, const int mask)
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = mask;
    
    sigaction(SIG,  &mystruct, (void*)NULL);
}


void reg_stupidhandler2_with_sig(const int SIG)
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = 0x0;
    
    sigaction(SIG,  &mystruct, (void*)NULL);
}


void reg_stupidhandler1()
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0x0;
    
    sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);
}

void reg_stupidhandler2()
{
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = 0x0;
    
    sigaction(MY_OTHER_SIGIG,  &mystruct, (void*)NULL);
}

void test_inherit_mask()
{
    int childid;
    childid = fork();
    struct sigaction mystruct;
    
    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = (1 << (MY_SIGSIG));

    sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);

    if (childid == 0)
    {
        // In child we should halt for a while.
        reg_stupidhandler1();

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
        mystruct.sigmask = (1 << (MY_SIGSIG));

        sigaction(MY_SIGSIG,  &mystruct, (void*)NULL);

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father\n");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(3);
        verbose_log(1, "sending sigsig to child\n");

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
        reg_stupidhandler1();

        // loop for a while?
        dummy_loop();        
    }
    else
    {

        verbose_log(0, "Entering father\n");
        // In parent
        verbose_log(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        verbose_log(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);

        verbose_log(1, "setting child kill sig!\n");

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
        reg_stupidhandler1();

        reg_stupidhandler2();
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

        verbose_log(1, "sending sigsig to child\n");

        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);

        // Sending Continue
        kill (childid, SIGCONT);

        kill(childid, MY_OTHER_SIGIG);
    }
    
}


void test_handler_suspended_no_cont()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        reg_stupidhandler1();

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


void test_n_fork(int forknum)
{
    int* pid_arr = malloc(sizeof(int) * forknum);
    int index = 0;

    for (index = 0; index < forknum; index++)
    {
        pid_arr[index] = fork();
        if (pid_arr[index] == 0)
        {
            dummy_loop();
        }
        if (pid_arr[index] > 0)
        {
            
            // Parent scope
        }
        else
        {
            // Error?

        }
        
    }

    while (wait() > 0);
    free(pid_arr);
}

int random_math_loop()
{
    uint mathnum = 0;
    uint numnum = getpid();
    numnum = numnum * numnum;
    
    while (mathnum < numnum)
    {
        if ((mathnum * mathnum) == numnum)
        {
            break;
        }
        mathnum++;
    }

    if (mathnum > numnum)
        return 0;
    return 1;
}

int main()
{
    test_handler_suspended();

    while ((wait()) > 0)
        ;

    exit();
}

