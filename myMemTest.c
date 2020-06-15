#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "param.h"
#include "fcntl.h"
#include "syscall.h"
#include "memlayout.h"
#include "mmu.h"

#define PGSIZE 4096
#define ARR_SIZE 82000
#define N_PAGES 24

char *data[N_PAGES];

void bigTest()
{
    int i = 0;
    int n = N_PAGES;

    for (i = 0; i < n;)
    {
        data[i] = sbrk(PGSIZE);
        data[i][0] = 00 + i;
        data[i][1] = 10 + i;
        data[i][2] = 20 + i;
        data[i][3] = 30 + i;
        data[i][4] = 40 + i;
        data[i][5] = 50 + i;
        data[i][6] = 60 + i;
        data[i][7] = 70 + i;
        printf(1, "allocated new page #%d at address: %x\n", i, data[i]);
        i++;
    }

    printf(1, "\nIterate through pages seq:\n");

    int j;
    for (j = 1; j < n; j++)
    {
        printf(1, "j:  %d\n", j);

        for (i = 0; i < j; i++)
        {
            data[i][10] = 2; // access to the i-th page
            printf(1, "%d, ", i);
        }
        printf(1, "\n");
    }

    int k;
    int pid = fork();
    if (pid)
        wait();
    else
    {
        printf(1, "\nGo through same 8 pages and different 8 others\n");
        for (j = 0; j < 8; j++)
        {
            for (i = 20; i < 24; i++)
            {
                data[i][10] = 1;
                printf(1, "%d, ", i);
            }
            printf(1, "\n");
            switch (j % 4)
            {
            case 0:
                for (k = 0; k < 4; k++)
                {
                    data[k][10] = 1;
                    printf(1, "%d, ", k);
                }
                break;
            case 1:
                for (k = 4; k < 8; k++)
                {
                    data[k][10] = 1;
                    printf(1, "%d, ", k);
                }
                break;
            case 2:
                for (k = 8; k < 12; k++)
                {
                    data[k][10] = 1;
                    printf(1, "%d, ", k);
                }
                break;
            case 3:
                for (k = 12; k < 16; k++)
                {
                    data[k][10] = 1;
                    printf(1, "%d, ", k);
                }
                break;
            }

            // data[j][10] = 0;
            // printf(1,"%d, ",j);
            printf(1, "\n");
        }
    }
}

/*
	Test used to check the swapping machanism in fork.
	Best tested when LIFO is used (for more swaps)
*/
void forkTest()
{
    int i;
    char *arr;
    arr = malloc(82000); //allocates 15 pages (sums to 16), in lifo, OS puts page #15 in file.

    for (i = 0; i < 50; i++)
    {
        arr[49100 + i] = 'A'; //last six A's stored in page #16, the rest in #15
        arr[45200 + i] = 'B'; //all B's are stored in page #15.
    }
    arr[49100 + i] = 0; //for null terminating string...
    arr[45200 + i] = 0;

    if (fork() == 0)
    { //is son
        sleep(1500);
        for (i = 40; i < 50; i++)
        {
            arr[49100 + i] = 'C'; //changes last ten A's to C
            arr[45200 + i] = 'D'; //changes last ten B's to D
        }
        printf(1, "Excpecting AAA... CCC...\n");
        printf(1, "SON: %s\n", &arr[49100]); // should print AAAAA..CCC...
        printf(1, "Excpecting BBB... DDD...\n");
        printf(1, "SON: %s\n", &arr[45200]); // should print BBBBB..DDD...
        printf(1, "\n");
        sleep(1500);
        free(arr);
        exit();
    }
    else
    { //is parent
        wait();
        printf(1, "Excpecting AAA...\n");

        printf(1, "PARENT: %s\n", &arr[49100]); // should print AAAAA...

        printf(1, "Excpecting BBB...\n");

        printf(1, "PARENT: %s\n", &arr[45200]); // should print BBBBB...

        sleep(1500);
        free(arr);
    }
}

static unsigned long int next = 1;
int getRandNum()
{
    next = next * 1103515245 + 12341;
    return (unsigned int)(next / 65536) % ARR_SIZE;
}

#define PAGE_NUM(addr) ((uint)(addr) & ~0xFFF)
#define TEST_POOL 500
/*
Global Test:
Allocates 17 pages (1 code, 1 space, 1 stack, 14 malloc)
Using pseudoRNG to access a single cell in the array and put a number in it.
Idea behind the algorithm:
	Space page will be swapped out sooner or later with scfifo or lap.
	Since no one calls the space page, an extra page is needed to play with swapping (hence the #17).
	We selected a single page and reduced its page calls to see if scfifo and lap will become more efficient.
Results (for TEST_POOL = 500):
LIFO: 42 Page faults
LAP: 18 Page faults
SCFIFO: 35 Page faults
*/
void globalTest()
{
    char *arr;
    int i;
    int randNum;
    arr = malloc(ARR_SIZE); //allocates 14 pages (sums to 17 - to allow more then one swapping in scfifo)
    for (i = 0; i < TEST_POOL; i++)
    {
        randNum = getRandNum(); //generates a pseudo random number between 0 and ARR_SIZE
        while (PGSIZE * 10 - 8 < randNum && randNum < PGSIZE * 10 + PGSIZE / 2 - 8)
            randNum = getRandNum(); //gives page #13 50% less chance of being selected
                                    //(redraw number if randNum is in the first half of page #13)
        arr[randNum] = 'X';         //write to memory
    }
    printf(1, "Ctrl+P now \n");

    sleep(2000);
    free(arr);
}

int main(int argc, char *argv[])
{
    // printf(1, "Running global test\n");
    // globalTest(); //for testing each policy efficiency
    // printf(1, "Running mini fork test\n");
    // forkTest(); //for testing swapping machanism in fork.
    printf(1, "running big test \n");
    bigTest();
    exit();
}