#include <stdio.h> 
#include <stdlib.h>
#include "param.h"
#include "mmu.h" 
#include "types.h"
#include "proc.h"

  
 
typedef struct node { 
    
    struct proc* data;
    // Lower values indicate higher priority
    //1 = highest priority
    //10 = lowest priority  
    int priority; 
  
    struct node* next; 
  
} Node;  

// Function to Create A New node
Node* newNode(struct proc* data, int priority) 
{ 
    Node* temp = (Node*)malloc(sizeof(Node)); 
    temp->data = data; 
    temp->priority = priority; 
    temp->next = NULL; 
  
    return temp; 
} 
  
// Return the priority at head 
int peek(Node** head) 
{ 
    return (*head)->priority; 
} 
  
// Removes the element with the 
// highest priority form the list 
struct proc* pop(Node** head) 
{ 
    Node* temp = *head; 
    struct proc* top_priority_proc=temp->data;
    (*head) = (*head)->next; 
    free(temp); 
    return top_priority_proc;
} 

// Function to push according to priority 
void push(Node** head,struct proc* data, int priority) 
{ 
    Node* start = (*head); 
  
    // Create new Node 
    Node* temp = newNode(data, priority); 
  
    // Special Case: The head of list has lesser 
    // priority than new node. So insert new 
    // node before head node and change head node. 
    if ((*head)->priority < priority) { 
  
        // Insert New Node before head 
        temp->next = *head; 
        (*head) = temp; 
    } 
    else { 
  
        // Traverse the list and find a 
        // position to insert new node 
        while (start->next != NULL && 
               start->next->priority < priority) { 
            start = start->next; 
        } 
  
        // Either at the ends of the list 
        // or at required position 
        temp->next = start->next; 
        start->next = temp; 
    } 
} 

// Function to check is list is empty 
int isEmpty(Node** head) 
{ 
    return (*head) == NULL; 
} 