#include <stdio.h>      //if you don't use scanf/printf change this include
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

struct process
{
    int ID;
    int Arr;
    int RunTime;
    int priority;
    int WaitingTime;
    int StartTime;
    int RemainingTime; //initially equals the running time.
    int turnaroundtime;
   // int Wturnaroundtime;
    int State; // 1 for running, 0 for waiting.
    void *Pshmaddr;
    int Psem1;
    int Psem2;
};

typedef short bool;
#define true 1
#define false 0

#define SHKEY 300


///==============================
//don't mess with this variable//
int * shmaddr;                 //
//===============================
#define LEFTCHILD(x) 2 * x + 1
#define RIGHTCHILD(x) 2 * x + 2
#define PARENT(x) (x - 1) / 2

struct PriorityQueue
{
    struct process *elements;
    int algoNo;
    int size;
};

struct PriorityQueue* PriorityQueue_init(int algoNo)
{
    struct PriorityQueue* priorityQ= malloc(sizeof(struct PriorityQueue));
    priorityQ->size = 0;
    priorityQ->algoNo = algoNo;
    return priorityQ;
}

void heapify(struct PriorityQueue *priorityQ, int i)
{
    if (priorityQ->algoNo == 1) // HPF    (priority)
    {
        int smallest = (LEFTCHILD(i) < priorityQ->size && priorityQ->elements[LEFTCHILD(i)].priority < priorityQ->elements[i].priority) ? LEFTCHILD(i) : i;
        if (RIGHTCHILD(i) < priorityQ->size && priorityQ->elements[RIGHTCHILD(i)].priority < priorityQ->elements[smallest].priority)
        {
            smallest = RIGHTCHILD(i);
        }
        if (smallest != i)
        {
           struct process temp=priorityQ->elements[i];
            priorityQ->elements[i]=priorityQ->elements[smallest];
            priorityQ->elements[smallest]=temp;
            heapify(priorityQ, smallest);
        }
    }
    else if (priorityQ->algoNo == 2)// SRTN      
    {
        int smallest = (LEFTCHILD(i) < priorityQ->size && priorityQ->elements[LEFTCHILD(i)].RemainingTime < priorityQ->elements[i].RemainingTime) ? LEFTCHILD(i) : i;
        if (RIGHTCHILD(i) < priorityQ->size && priorityQ->elements[RIGHTCHILD(i)].RemainingTime < priorityQ->elements[smallest].RemainingTime)
        {
            smallest = RIGHTCHILD(i);
        }
        if (smallest != i)
        { 
            struct process temp=priorityQ->elements[i];
            priorityQ->elements[i]=priorityQ->elements[smallest];
            priorityQ->elements[smallest]=temp;
            heapify(priorityQ, smallest);
        }
    }
}

void push(struct PriorityQueue *priorityQ,struct process *ProcessToBePushed)
{
    if (priorityQ->size > 0)
    {
        priorityQ->elements = realloc(priorityQ->elements, (priorityQ->size + 1) * sizeof(struct process));
    }
    else
    {
        priorityQ->elements = malloc(sizeof(struct process));
    }

    struct process NewProcess;
    //Our process struct data

    NewProcess.priority = ProcessToBePushed->priority;
    NewProcess.RemainingTime = ProcessToBePushed->RemainingTime;
    NewProcess.ID = ProcessToBePushed->ID;

    NewProcess.WaitingTime = ProcessToBePushed->WaitingTime;
    NewProcess.StartTime = ProcessToBePushed->StartTime;
    NewProcess.State = ProcessToBePushed->State;
   
    NewProcess.Arr = ProcessToBePushed->Arr;
    NewProcess.RunTime = ProcessToBePushed->RunTime;
    NewProcess.Pshmaddr = ProcessToBePushed->Pshmaddr;

    NewProcess.Psem1 = ProcessToBePushed->Psem1;
    NewProcess.Psem2 = ProcessToBePushed->Psem2;
  
    int index = (priorityQ->size);
    (priorityQ->size)++;
    if (priorityQ->algoNo == 1) // HPF
    {
        while (index && NewProcess.priority < priorityQ->elements[PARENT(index)].priority)
        {
            priorityQ->elements[index] = priorityQ->elements[PARENT(index)];
            index = PARENT(index);
        }
        priorityQ->elements[index] = NewProcess;
    }
    else if (priorityQ->algoNo == 2)// SRTN
    {
        while (index && NewProcess.RemainingTime < priorityQ->elements[PARENT(index)].RemainingTime)
        {
           if ( priorityQ->elements[PARENT(index)].State==1)
                  priorityQ->elements[PARENT(index)].WaitingTime+=NewProcess.RemainingTime;
            priorityQ->elements[index] = priorityQ->elements[PARENT(index)];
            index = PARENT(index);
        }
        priorityQ->elements[index] = NewProcess;
    }
}

int Empty(struct PriorityQueue *priorityQ)
{
    return priorityQ->size == 0;
}

struct process *pop(struct PriorityQueue *priorityQ) //return element of highest priprity anf pop it
{
    if (priorityQ->size > 0)
    {
        struct process newProcess;
        ///Our process struct elements
        newProcess.priority = priorityQ->elements[0].priority;
    newProcess.RemainingTime = priorityQ->elements[0].RemainingTime;
    newProcess.ID = priorityQ->elements[0].ID;

    newProcess.WaitingTime = priorityQ->elements[0].WaitingTime;
    newProcess.StartTime = priorityQ->elements[0].StartTime;
    newProcess.State = priorityQ->elements[0].State;
   
    newProcess.Arr = priorityQ->elements[0].Arr;
    newProcess.RunTime = priorityQ->elements[0].RunTime;
    newProcess.Pshmaddr = priorityQ->elements[0].Pshmaddr;

    newProcess.Psem1 = priorityQ->elements[0].Psem1;
    newProcess.Psem2 = priorityQ->elements[0].Psem2;

        struct process *temp = &newProcess;

        priorityQ->elements[0] = priorityQ->elements[--(priorityQ->size)]; //put last element in the first element place --> heapify
        priorityQ->elements = realloc(priorityQ->elements, priorityQ->size * sizeof(struct process));
        heapify(priorityQ, 0);
        return temp;
    }
    else
    {
        free(priorityQ->elements);
        return NULL;
    }
}

struct process *peek(struct PriorityQueue *priorityQ) //return element of highest priority without poping it
{
    if (priorityQ->size > 0)
    {
        struct process newProcess;
        ///Our process struct elements
        newProcess.priority = priorityQ->elements[0].priority;
        newProcess.RemainingTime = priorityQ->elements[0].RemainingTime;
        newProcess.ID = priorityQ->elements[0].ID;

        newProcess.WaitingTime = priorityQ->elements[0].WaitingTime;
        newProcess.StartTime = priorityQ->elements[0].StartTime;
        newProcess.State = priorityQ->elements[0].State;
    
        newProcess.Arr = priorityQ->elements[0].Arr;
        newProcess.RunTime = priorityQ->elements[0].RunTime;
        newProcess.Pshmaddr = priorityQ->elements[0].Pshmaddr;

        newProcess.Psem1 = priorityQ->elements[0].Psem1;
        newProcess.Psem2 = priorityQ->elements[0].Psem2;

        struct process *temp = &newProcess;
        return temp;
    }
    else
        return NULL;
}

//=================================================
struct Node
{
    struct process *data;
    struct Node *next;
};

struct Queue
{
    struct Node *front;
    struct Node *rear;
    int size;
};
struct Node *newNode(struct process *data)
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));
    node->data = data;
    node->next = NULL;
    return node;
}

struct Queue* Queue_init()
{
    struct Queue* q=malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}


struct process* dequeue(struct Queue *q)
{
    if (q->size == 0)
        return NULL;
    struct Node *temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL)
        q->rear = NULL;  //empty queue

    q->size -= 1;

    return temp->data;
}

void enqueue(struct Queue *q,struct process *enqueuedProcess)
{
    struct Node *node = newNode(enqueuedProcess);
    q->size += 1;
    if (q->rear == NULL) 
    {
        q->front = q->rear = node;
    } 
    else
    {
        q->rear->next = node;
        q->rear = node;
    }
    
}

bool isEmpty(struct Queue *q)
{
    return (q->size == 0);
}

struct process *front(struct Queue *q)
{
    if (q->front != NULL)
        return q->front->data;
    else
        return NULL;
}
//=================================================
int getClk()
{
    return *shmaddr;
}
/*
 * All process call this function at the beginning to establish communication between them and the clock module.
 * Again, remember that the clock is only emulation!
*/
void initClk()
{
    int shmid = shmget(SHKEY, 4, 0444);
    while ((int)shmid == -1)
    {
        //Make sure that the clock exists
        printf("Wait! The clock not initialized yet!\n");
        sleep(1);
        shmid = shmget(SHKEY, 4, 0444);
    }
    shmaddr = (int *) shmat(shmid, (void *)0, 0);
}


/*
 * All process call this function at the end to release the communication
 * resources between them and the clock module.
 * Again, Remember that the clock is only emulation!
 * Input: terminateAll: a flag to indicate whether that this is the end of simulation.
 *                      It terminates the whole system and releases resources.
*/

void destroyClk(bool terminateAll)
{
    shmdt(shmaddr);
    if (terminateAll)
    {
        killpg(getpgrp(), SIGINT);
    }
}
