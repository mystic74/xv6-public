#include <stdio.h>
#include <stdlib.h>
#include "param.h"
#include "mmu.h"
#include "types.h"
#include "spinlock.h"
#include "proc.h"

typedef struct node
{

    struct proc *data;
    // Lower values indicate higher priority
    //1 = highest priority
    //10 = lowest priority
    int priority;
    int init;
    struct node *next;

} Node;

struct
{
    struct spinlock lock;
    Node node_arr[NPROC];
} node_mem;

Node *getmem()
{
    int index = 0;
    // Go thorugh the array and find an empty node
    for (index = 0; index < NPROC; index++)
    {
        if (node_mem.node_arr[index].init == 0)
        {
            node_mem.node_arr[index].init = 1;
            return &(node_mem.node_arr[index]);
        }
    }
    return 0;
}

int release_mem(void *p)
{
    int index = 0;
    // Go thorugh the array and find an empty node
    for (index = 0; index < NPROC; index++)
    {
        if (&(node_mem.node_arr[index]) == p)
        {
            node_mem.node_arr[index].init = 0;
            return 0;
        }
    }
    return 1;
}
// Function to Create A New node
Node *newNode(struct proc *data, int priority)
{
    // TomR : Check to see if we got something? can be a filled up array.
    Node *temp = (Node *)getmem();

    temp->data = data;
    temp->priority = priority;
    temp->next = NULL;

    return temp;
}

// Return the priority at head
int peek(Node **head)
{
    return (*head)->priority;
}

// Removes the element with the
// highest priority form the list
struct proc *pop(Node **head)
{
    Node *temp = *head;
    struct proc *top_priority_proc = temp->data;
    (*head) = (*head)->next;
    release_mem(temp);
    return top_priority_proc;
}

// Function to push according to priority
void push(Node **head, struct proc *data, int priority)
{
    Node *start = (*head);

    // Create new Node
    Node *temp = newNode(data, priority);

    // Special Case: The head of list has lesser
    // priority than new node. So insert new
    // node before head node and change head node.
    if ((*head)->priority < priority)
    {

        // Insert New Node before head
        temp->next = *head;
        (*head) = temp;
    }
    else
    {

        // Traverse the list and find a
        // position to insert new node
        while (start->next != NULL &&
               start->next->priority < priority)
        {
            start = start->next;
        }

        // Either at the ends of the list
        // or at required position
        temp->next = start->next;
        start->next = temp;
    }
}

// Function to check is list is empty
int isEmpty(Node **head)
{
    return (*head) == NULL;
}