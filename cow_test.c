#include "types.h"
#include "stat.h"
#include "user.h"


int shared_var1 = 0;
int shared_var2 = 0;

void test1()
{
    
    printf(1,"number of free pages: %d\n", getNumberOfFreePages());
   
    int pid = fork();
    
    if (pid == 0)
    {
        printf(1,"number of free pages after fork: %d\n", getNumberOfFreePages());
        //printf(1, "child: shared var = %d\n", shared_var1);
        printf(1, "child: changing shared var to 1\n");
        shared_var1 = 1;
        //printf(1, "child: shared var = %d\n", shared_var1);
        printf(1,"child: number of free pages after change: %d\n", getNumberOfFreePages());
        exit();
    }
    wait();
    printf(1,"parent: number of free pages after wait: %d\n", getNumberOfFreePages());
    //printf(1, "parent: shared var = %d\n", shared_var1);
    
    
}

void test2()
{
    printf(1,"number of free pages: %d\n", getNumberOfFreePages());
    if(fork()==0)
    {
        printf(1,"number of free pages after fork 1: %d\n", getNumberOfFreePages());
        //printf(1, "child1: shared var = %d\n", shared_var2);
        printf(1, "child1: changing shared var to 1\n");
        shared_var2 = 1;
        //printf(1, "child1: shared var = %d\n", shared_var2);
        printf(1,"child1: number of free pages after change: %d\n", getNumberOfFreePages());
        exit();
    }
    else
    {
        wait();
        
        printf(1,"number of free pages before fork 2: %d\n", getNumberOfFreePages());
        if(fork()==0)
        {
            printf(1,"number of free pages after fork 2: %d\n", getNumberOfFreePages());
            //printf(1, "child2: shared var = %d\n", shared_var2);
            printf(1, "child2: changing shared var to 2\n");
            shared_var2 = 2;
            //printf(1, "child2: shared var = %d\n", shared_var2);
            printf(1,"child2: number of free pages after change: %d\n", getNumberOfFreePages());
            exit();
        }
        
    }
    wait();
    printf(1,"parent2: number of free pages after wait child2: %d\n", getNumberOfFreePages());
    
}


int main(void)
{
    printf(1,"Test1\n");
    test1();
    printf(1,"Test1 finished\n");


    printf(1,"Test2\n");
    test2();
    printf(1,"Test2 finished\n");
    exit();
}



