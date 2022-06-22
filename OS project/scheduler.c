#include "headers.h"
#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define null 0
int slotTime;
int startOfscheduler;
void terminate(int signum);
void RecieveFromGenerator(int signum);
void ProcessTerminates(int signum);
void Schedul(int noofprocesses,int algonum);
void RRAlgo();
void SRTF(int noofprocesses);
void HPF(int noofprocesses);
struct PriorityQueue *PCBarr;
struct Queue *RoundRopin;
int order;
int shmidProcessWithScheduler;
int k;
int startOfscheduler;
bool f;
int TA;
FILE * logFile;
struct process** PCB;
struct process dummyProcess;

struct msgbuff
{
    long mtype;
    struct process proc;
};
union Semun
{
    int val;               /* value for SETVAL */
    struct semid_ds *buf;  /* buffer for IPC_STAT & IPC_SET */
    ushort *array;         /* array for GETALL & SETALL */
    struct seminfo *__buf; /* buffer for IPC_INFO */
    void *__pad;
};

void down(int sem)
{
    struct sembuf p_op;

    p_op.sem_num = 0;
    p_op.sem_op = -1;
    p_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &p_op, 1) == -1)
    {
        perror("Error in down()");
        exit(-1);
    }
}

void up(int sem)
{
    struct sembuf v_op;

    v_op.sem_num = 0;
    v_op.sem_op = 1;
    v_op.sem_flg = !IPC_NOWAIT;

    if (semop(sem, &v_op, 1) == -1)
    {
        perror("Error in up()");
        exit(-1);
    }
}

int main(int argc, char * argv[])
{   k=0;
    signal(SIGINT, terminate);
    signal(SIGUSR1, RecieveFromGenerator);
    signal(SIGCHLD,ProcessTerminates);
    printf("Hello from Scheduler\n");
     
    int NoOfProcesses=atoi(argv[1]);
    int AlgoNumber=atoi(argv[2]);
    slotTime=atoi(argv[3]);
    PCBarr=PriorityQueue_init(AlgoNumber);

  
    fflush(stdout);
    printf("Number of processes %d\n",NoOfProcesses);
    fflush(stdout);
    printf("Number of Algorithm %d\n",AlgoNumber);
    fflush(stdout);
    printf("Quamtum period %d\n",slotTime);
    fflush(stdout);
    PCB=(struct process**)malloc(NoOfProcesses*sizeof(struct process*));
    initClk();
    // To get time use this
    startOfscheduler = getClk();

    printf("///////////////////////\n");
    startOfscheduler=getClk();
     while(1){
       if(!Empty(PCBarr))
       {
            logFile = fopen("Scheduler.log", "w");
            fprintf(logFile, "At time\tx\tprocess\ty\tstate\tarr\tw\ttotal\tz\tremain\ty\twait\tk\n");
          Schedul(NoOfProcesses,AlgoNumber) ;
           fclose(logFile);
       }
       if (f)  
           break;
     }
    
    fflush(stdout);
    printf("Scheduler: Bye\n");
    destroyClk(true);
}

void RecieveFromGenerator(int signum)
{
    fflush(stdout);
    int x = getClk();
    printf("%d\n",x);
    
    key_t key= ftok("Secondfile", getppid()); 
    int msgq_id = msgget(key, 0666 | IPC_CREAT); //create message queue and return id
    if (msgq_id == -1)
    {
        fflush(stdout);
        perror("Scheduler:Error in create message queue\n");
        exit(-1);
    }
    struct msgbuff message;
    //Recieve the nessage and check it is recieved
    int rec_val = msgrcv(msgq_id, &message,sizeof(message.proc),1, !IPC_NOWAIT);
    if (rec_val == -1)
    {
        fflush(stdout);
        perror("Error in receive\n");
        exit(-1);
    }
    //Create new object of struct PCB and fill it with its info
    ////////////////////////////////////////////////////////////////
    struct process *newp=&message.proc;
    ////////////////////////////////////////////////////////////////
    //Fork the new process and create shared memory and 2 semaphores with it
    
    int ProcessId=fork();
    if(ProcessId==-1)
    {
        fflush(stdout);
        printf("\nError in fork process\n");
    }
    else if (ProcessId==0)
    {
        char Proc[10];
        sprintf(Proc,"%d",message.proc.ID);
        int result=execl("process.out","process.out",Proc,NULL);
/////////////////////////////////////////////////////////////////////////////
        if(result==-1)
        {
            fflush(stdout);
            printf("\nExecl process failed\n");
        }
        
    }
    else
    {
        fflush(stdout);
        printf("\nfrom scahjoio\n");
        //create shared memory with each process to share the remaining time
        shmidProcessWithScheduler = shmget(ProcessId, 256, IPC_CREAT | 0644);

        if (shmidProcessWithScheduler == -1)
        {
            fflush(stdout);
            perror("Scheduler:Error in create Shm with process");
            exit(-1);
        }
        void *shmAddr = shmat(shmidProcessWithScheduler, (void *)0, 0);
        if ((long)shmAddr == -1)
        {
            fflush(stdout);
            perror("Error in attach in writer");
            exit(-1);
        }
        //2 
        key_t key_id1= ftok("Firstfile", ProcessId); 
        key_t key_id2= ftok("Secondfile", ProcessId); 
        key_t key_id= ftok("Secondfile", getppid()); 
        int semComm = semget(key_id, 1, 0666 | IPC_CREAT);
        int sem1 = semget(key_id1, 1, 0666 | IPC_CREAT);
        int sem2 = semget(key_id2, 1, 0666 | IPC_CREAT);
        if (sem1 == -1 || sem2 == -1)
        {
            fflush(stdout);
            perror("Process:Error in create sem");
            exit(-1);
        }
        union Semun semun;
        semun.val = 1; /* initial value of the semaphore, Binary semaphore */
        if (semctl(sem1, 0, SETVAL, semun) == -1) // set initial value of sem 0 in the sem1 set = semun.val
        {
            fflush(stdout);
            perror("Scheduler:Error in semctl sem1");
            exit(-1);
        }
        if (semctl(sem2, 0, SETVAL, semun) == -1)
        {
            fflush(stdout);
            perror("Scheduler:Error in semctl sem2");
            exit(-1);
        }
        semun.val = 0;
        if (semctl(semComm, 0, SETVAL, semun) == -1) // set initial value of sem 0 in the sem1 set = semun.val
        {
            fflush(stdout);
            perror("Scheduler:Error in semctl semComm");
            exit(-1);
        }
        newp->Psem1=sem1;
        newp->Psem2=sem2;
        newp->Pshmaddr=shmAddr;
        PCB[(newp->ID)-1]=newp;
        push(PCBarr,newp);
        //fflush(stdout);
        //printf("%d\n",getClk());
        int remainingtime=message.proc.RemainingTime; // For test
        /////after change remaining time put the following 3 lines/////
        char remainingtimeChar[10];
        sprintf(remainingtimeChar,"%d",remainingtime);
        strcpy((char *) shmAddr,remainingtimeChar);
        down(sem1);
        up(sem2);//When it wants to read it
        up(semComm);
    }
}
void Schedul(int noofprocesses,int algonum)
{ 
    //HPF
    if (algonum==1)
    {
      HPF(noofprocesses);
    }
    //SRTF
    else if(algonum==2)
    {
       SRTF(noofprocesses);  
     }
    
    //RR
    else
    {
        RRAlgo();
    }
}

void SRTF(int noofprocesses)
{
    fflush(stdout);
    printf("Hello From SRTF\n");

    while(!Empty(PCBarr))
    {
        int runt=0;
        k++;
        struct process *prun ;
        prun=peek(PCBarr) ; 
        if ( prun->State==0)
        {
            runt=prun->RunTime;
            prun->StartTime=getClk(); 
            fprintf(logFile,"At time\t%d\t%d\tstarted\t%d\t%d\t%d\t%d\n", prun->StartTime, prun->ID,0, prun->RunTime,prun->RemainingTime,prun->WaitingTime); 
            prun->State=1;
        }
        else
        {
            runt=prun->RemainingTime;
            fprintf(logFile, "At time\t%d\t%d\tresumed\t%d\t%d\t%d\t%d\n",getClk(), prun->ID,0, prun->RunTime,prun->RemainingTime,prun->WaitingTime);
        }      
        while(getClk()-prun->StartTime< runt)
        {
            prun->RemainingTime--;
            //send to process remining time
            
            char remainingtimeChar[10];
            sprintf(remainingtimeChar,"%d",prun->RemainingTime);
            strcpy((char *) prun->Pshmaddr,remainingtimeChar);
        }  
        struct process *pfin ;
        pfin=pop(PCBarr) ; 
        fprintf(logFile, "At time\t%d\t%d\tfinished\t%d\t%d\t%d\t%d\n",getClk(), prun->ID,0, prun->RunTime,prun->RemainingTime,prun->WaitingTime);
        if(k==noofprocesses)    
        f=true;  
    }
}
void HPF(int noofprocesses)
{
   while(!Empty(PCBarr))
         {
             k++;
             struct process *prun ;
             prun=pop(PCBarr) ; 
             prun->StartTime=getClk();   
             fprintf(logFile, "At time\t%d\t%d\tStarted\t%d\t%d\t%d\t%d\n", prun->StartTime, prun->ID,0, prun->RunTime,prun->RemainingTime,prun->WaitingTime); 
              while((getClk()-prun->StartTime)<prun->RunTime)
         {
            prun->RemainingTime--;
            //send to process remining time
            
            char remainingtimeChar[10];
            sprintf(remainingtimeChar,"%d",prun->RemainingTime);
            strcpy((char *) prun->Pshmaddr,remainingtimeChar);
         } 

          fprintf(logFile, "At time\t%d\t%d\tfinished\t%d\t%d\t%d\t%d\n",getClk(), prun->ID,0, prun->RunTime,prun->RemainingTime,prun->WaitingTime);
          if(k==noofprocesses)    
           f=true;         
         }

}
void RRAlgo()
{
    int ThisSlot=0;
    while(front(RoundRopin)!=NULL)
    {
        struct process* ProcessToBeScheduled =dequeue(RoundRopin);
        ProcessToBeScheduled->State=1; //running
        if(ProcessToBeScheduled->StartTime==0)
        {
            fprintf(logFile, "At time\t%d\t%d\tstarted\t%d\t%d\t%d\t%d\n",getClk(), ProcessToBeScheduled->ID,0, ProcessToBeScheduled->RunTime,ProcessToBeScheduled->RemainingTime,ProcessToBeScheduled->WaitingTime);
                ProcessToBeScheduled->StartTime=getClk();
        }
       else
        {
           fprintf(logFile, "At time\t%d\t%d\tresumed\t%d\t%d\t%d\t%d\n",getClk(), ProcessToBeScheduled->ID,0, ProcessToBeScheduled->RunTime,ProcessToBeScheduled->RemainingTime,ProcessToBeScheduled->WaitingTime);
        }
        //printf("\n state %d",PCBarr[i]->state);
        ProcessToBeScheduled->WaitingTime=getClk()-ProcessToBeScheduled->Arr;
        //printf("\n waiting %d",PCBarr[i]->waitingtime);
        if(slotTime>ProcessToBeScheduled->RemainingTime)
        {
            ThisSlot=ProcessToBeScheduled->RemainingTime;
        }
        else
        {
            ThisSlot=slotTime;
        }
        printf("\n thisslot %d",ThisSlot);
        int start=getClk();
        while(1)
        {
            if(getClk()-start==ThisSlot)
            break;
        }
        ProcessToBeScheduled->RemainingTime=ProcessToBeScheduled->RemainingTime-ThisSlot;
        int remainingtime=ProcessToBeScheduled->RemainingTime;
        char remainingtimeChar[10];
        sprintf(remainingtimeChar,"%d",remainingtime);
        strcpy((char *) ProcessToBeScheduled->Pshmaddr,remainingtimeChar);
        down(ProcessToBeScheduled->Psem1);
        up(ProcessToBeScheduled->Psem2);//When it wants to read it
        if(ProcessToBeScheduled->RemainingTime==0)
        {
            TA=getClk()-ProcessToBeScheduled->Arr;
            fprintf(logFile, "At time\t%d\t%d\tfinished\t%d\t%d\t%d\t%d\n",getClk(), ProcessToBeScheduled->ID,0, ProcessToBeScheduled->RunTime,ProcessToBeScheduled->RemainingTime,ProcessToBeScheduled->WaitingTime);
        }
        else
        {
            fprintf(logFile, "At time\t%d\t%d\tstopped\t%d\t%d\t%d\t%d\n",getClk(), ProcessToBeScheduled->ID,0, ProcessToBeScheduled->RunTime,ProcessToBeScheduled->RemainingTime,ProcessToBeScheduled->WaitingTime);
            ProcessToBeScheduled->State=0; //waiting
            enqueue(RoundRopin,ProcessToBeScheduled);
        }
        
    }
    
}

void ProcessTerminates(int signum)
{
    int TerminatedProcessID;
    int stat_loc;
    int sid = wait(&stat_loc);
    if(!(stat_loc & 0x00FF))
    {
        TerminatedProcessID=stat_loc>>8;
        PCB[TerminatedProcessID-1]=&dummyProcess;
    }
}


void terminate(int signum)
{
    killpg(getpgrp(),SIGINT);//send SIGINT to all processes
    exit(0);
}

