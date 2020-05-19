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

int pid_array[proc_number + 1];
void init_pid_array()
{
    int i;
    for (i = 0; i < proc_number + 1; i++)
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

void dummy_action()
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0x0;

    printf(1, " address is %x, setting in param \n", &stupid_handler1);
    printf(1, "This is the action registering function for proc :%d  \n", getpid());

    sigaction(MY_SIGSIG, &mystruct, (void *)NULL);
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

void reg_handler_with_sig_and_mask(const void *new_handler, const int SIG, const int mask)
{
    struct sigaction mystruct;

    mystruct.sa_handler = new_handler;
    mystruct.sigmask = mask;

    sigaction(SIG, &mystruct, (void *)NULL);
}

void reg_stupidhandler2_with_sig_and_mask(const int SIG, const int mask)
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = mask;

    sigaction(SIG, &mystruct, (void *)NULL);
}

void reg_stupidhandler2_with_sig(const int SIG)
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = 0x0;

    sigaction(SIG, &mystruct, (void *)NULL);
}

void reg_stupidhandler1()
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0x0;

    sigaction(MY_SIGSIG, &mystruct, (void *)NULL);
}

void reg_stupidhandler2()
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = 0x0;

    sigaction(MY_OTHER_SIGIG, &mystruct, (void *)NULL);
}

void dummy_loop()
{
    uint i = 0xBABE;
    int uptime_org = uptime();
    int uptime_digit = uptime_org % 100;

    uint dummy = 0;
    while (i--)
        dummy += i;

    sleep(uptime_digit);

    sleep(400);
    printf(1, "testprint for %d! \n", getpid());
    sleep(600);
    while (dummy--)
        i++;
    printf(1, "%d was here, slept for %x seconds, lived for %d ticks and died :( \n",
           getpid(), uptime_digit, uptime() - uptime_org);

    exit();
}

int find_pid(int curpid)
{
    int i;
    for (i = 1; i < proc_number + 1; i++)
    {
        if (pid_array[i] != curpid && pid_array[i] != -1)
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

        kill(send_to, SIGKILL);

        kill(send_to, SIGSTOP);

        kill(send_to, SIGCONT);

        kill(send_to - 1, MY_SIGSIG);
        kill(send_to, MY_SIGSIG);
    }
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

void test_inherit_mask()
{
    int childid;
    childid = fork();
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler2;
    mystruct.sigmask = (1 << (MY_SIGSIG));

    sigaction(MY_SIGSIG, &mystruct, (void *)NULL);

    if (childid == 0)
    {
        // In child we should halt for a while.
        reg_stupidhandler1();

        // loop for a while?
        dummy_loop();
    }
    else
    {

        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        printf(1, "setting child to pause\n");

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

        sigaction(MY_SIGSIG, &mystruct, (void *)NULL);

        // loop for a while?
        dummy_loop();
    }
    else
    {

        printf(1, "Entering father\n");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(3);
        printf(1, "sending sigsig to child\n");

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

        printf(1, "Entering father\n");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        printf(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);

        printf(1, "setting child kill sig!\n");

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

        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(500);
        printf(1, "setting child to pause and sleeping for 100ticks \n");

        // Sending stop to child.
        kill(childid, SIGSTOP);
        sleep(100);
        printf(1, "sending sigsig to child\n");

        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);
        printf(1, "setting child to continue\n");

        // Sending Continue
        kill(childid, SIGCONT);
        sleep(300);
        printf(1, "Sening another signal\n");
        kill(childid, MY_OTHER_SIGIG);
    }
}

void test_stop_overide_fail()
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0;
    int childid;
    childid = fork();

    if (childid == 0)
    {

        if (sigaction(SIGSTOP, &mystruct, (void *)NULL) < 0)
        {
            printf("sigaction for signal %d failed \n", SIGSTOP);
        }
        else
        {
            printf("sigaction for signal %d succeded \n", SIGSTOP);
        }
    }
}

void test_kill_overide_fail()
{
    struct sigaction mystruct;

    mystruct.sa_handler = &stupid_handler1;
    mystruct.sigmask = 0;
    int childid;
    childid = fork();

    if (childid == 0)
    {

        if (sigaction(SIGKILL, &mystruct, (void *)NULL) < 0)
        {
            printf("sigaction for signal %d failed \n", SIGKILL);
        }
        else
        {
            printf("sigaction for signal %d succeded \n", SIGKILL);
        }
    }
}

void test_handler_suspended_overwrite_cont()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        reg_handler_with_sig_and_mask((void *)SIGCONT, MY_SIGSIG, 0x0);

        reg_stupidhandler2_with_sig(SIGCONT);
        // loop for a while?
        dummy_loop();
    }
    else
    {

        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(100);
        printf(1, "setting child to pause and sleeping for 100 ticks \n");

        // Sending stop to child.
        kill(childid, SIGSTOP);
        sleep(100);

        printf(1, "sending sigsig to child (Now sig cont) \n");
        // sending the dumb signal now
        kill(childid, MY_SIGSIG);

        sleep(300);
        printf(1, "sending stupid sighandler2 \n");
        // Sending Continue
        kill(childid, SIGCONT);

        sleep(300);
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
        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(1);
        printf(1, "setting child to pause\n");

        // Sending stop to child.
        kill(childid, SIGSTOP);
        sleep(100);
        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);
    }
}

void test_double_regist_same_sig_print()
{
    int childid;
    childid = fork();

    if (childid == 0)
    {
        // In child we should halt for a while.
        reg_stupidhandler1();

        reg_stupidhandler2_with_sig(MY_SIGSIG);

        // loop for a while?
        dummy_loop();
    }
    else
    {
        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(100);
        printf(1, "sending the signal to the process \n");
        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);
    }
}

void test_double_regist_print()
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
        printf(1, "Entering father");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(100);
        printf(1, "sending the signal to the process \n");
        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);

        kill(childid, MY_OTHER_SIGIG);
    }
}

void test_basic_fork_and_sigsig()
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
        printf(1, "Entering father\n");
        // In parent
        printf(1, "sleeping for a second for syncing parent... \n");
        sleep(100);
        printf(1, "sending the signal to the process \n");
        // sending the dumb signal now, expecting NO PRINT.
        kill(childid, MY_SIGSIG);
    }
}

void test_basic_loop()
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

    printf(1, "Basic fork and signal: \n");
    test_basic_fork_and_sigsig();
    sleep(1000);

    printf(1, "\n\n\n\n\nBasic two signals: \n");
    test_double_regist_print();
    sleep(1000);

    printf(1, "\n\n\n\nTesting same sig registration\n");
    test_double_regist_same_sig_print();

    sleep(1000);

    printf(1, "\n\n\n Testin suspend with  cont \n");
    test_handler_suspended();

    sleep(1000);
    printf(1, "\n\n\n Testin suspend with overwrite cont \n");
    test_handler_suspended_overwrite_cont();

    test_kill_overide_fail();
    // sleep(1000);
    /*
        PLEASE NOTICE, THIS TEST DOENS'T END. AT ALL.
    */
    // printf(1, "\n\n\n Testin suspend with no cont \n");
    // test_handler_suspended_no_cont();

    while ((wait()) > 0)
        ;

    exit();
}
