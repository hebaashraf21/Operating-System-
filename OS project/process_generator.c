#include "headers.h"

void clearResources(int);
int pidScheduler; // To communicate with scheduler
int MyQueueId;
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
{
    int N=0; // Number of lines of the file 
    signal(SIGINT, clearResources);
    key_t key_id= ftok("Secondfile", getpid()); 
    int semComm = semget(key_id, 1, 0666 | IPC_CREAT);
    union Semun semun;
    semun.val = 0;
    if (semctl(semComm, 0, SETVAL, semun) == -1) // set initial value of sem 0 in the sem1 set = semun.val
    {
        fflush(stdout);
        perror("Generator:Error in semctl semComm");
        exit(-1);
    }
    // TODO Initialization
    // 1. Read the input files.
    FILE* inputFile = fopen("processes.txt","r");//sefine input file and open it
    if(inputFile==NULL)
    {
        fflush(stdout);
        perror("Error in open input file");
        exit(-1);
    }
    char OneLine[30];
    fgets(OneLine,sizeof(OneLine),inputFile);//ignore first line
    struct Queue* GeneratorQueue=malloc(sizeof(struct Queue));
    while(fgets(OneLine,sizeof(OneLine),inputFile))
    {
        N++;//No of processes
        struct process* MyProcess=malloc(sizeof(struct process));
        char*token =strtok(OneLine,"\t");
        MyProcess->ID=atoi(token);
        token=strtok(NULL,"\t");
        MyProcess->Arr=atoi(token);
        token=strtok(NULL,"\t");
        MyProcess->RunTime=atoi(token);
        token=strtok(NULL,"\t");
        MyProcess->priority=atoi(token); 
        //Fill the other members of the process;
        MyProcess->WaitingTime=0;
        MyProcess->StartTime=0;
        MyProcess->turnaroundtime=0;
        MyProcess->RemainingTime=MyProcess->RunTime;
        MyProcess->State=0;
        MyProcess->Pshmaddr=NULL;
        MyProcess->Psem1=0;
        MyProcess->Psem2=0;
        enqueue(GeneratorQueue,MyProcess);
    }
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int choice;
    int quantum=0;
    fflush(stdout);
    printf("\nEnter The number of algorithm to choose it(1,2 or 3):\n");
    fflush(stdout);
    printf("1- Highest Priority First\n");
    fflush(stdout);
    printf("2-Shortest ramaining time next\n");
    fflush(stdout);
    printf("3-Round Robin\n");
    scanf("%d",&choice);
    if(choice==3)
    {
        fflush(stdout);
        printf("\nYou chose Round Robin Algorithm\n");
        fflush(stdout);
        printf("Please Enter an integer number to be the quantum\n");
        scanf("%d",&quantum);
        while(quantum<=0)
        {
            fflush(stdout);
            printf("Please Enter a valid positive integer number to be the quantum\n");
            scanf("%d",&quantum);
        }
    }
    // 3. Initiate and create the scheduler and clock processes.
    int pidClk=fork();
    if(pidClk==-1)
    {
        fflush(stdout);
        printf("\nError in fork clock\n");
    }
    else if(pidClk==0)
    {
        //Child
        int result=execl("clk.out","clk.out",NULL);
        if(result==-1)
        {
            fflush(stdout);
            printf("\nExecl clk failed\n");
        }
    }
    else
    {
        //generator
        pidScheduler=fork();
        if(pidScheduler==-1)
        {
            fflush(stdout);
            printf("\nError in fork Scheduler\n");
        }
        else if(pidScheduler==0)
        {
            char NoOfProcesses[10];
            sprintf(NoOfProcesses,"%d",N);
            char AlgoNumber[10];
            sprintf(AlgoNumber,"%d",choice);
            char Quantum[10];
            sprintf(Quantum,"%d",quantum);
            int result=execl("scheduler.out","scheduler.out",NoOfProcesses,AlgoNumber,Quantum,NULL);
            if(result==-1)
            {
               fflush(stdout);
               printf("\nExecl scheduler failed\n");
            }
        }
    }
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
    int x = getClk();
    fflush(stdout);
    printf("current time is %d\n", x);
    //Create Message Queue with the scheduler
    key_t key= ftok("Secondfile", getpid()); 
    MyQueueId = msgget(key, 0666 | IPC_CREAT); //create message queue and return id
    if(MyQueueId==-1)
    {
        fflush(stdout);
        perror("Generator:Error in create message queue\n");
        exit(-1);
    }
    
    // 6. Send the information to the scheduler at the appropriate time.
    while(front(GeneratorQueue)!=NULL)
    {
        struct msgbuff message;
        message.mtype=1;//dummy
        if(front(GeneratorQueue)->Arr<=getClk()) //If the process arrived
        {
            struct process* ProcessToBeScheduled =dequeue(GeneratorQueue); //Remove it from the generator queue
            message.proc=*ProcessToBeScheduled;
            fflush(stdout);
            printf("Arrived At %d\n",getClk());
            fflush(stdout);
            int send_val = msgsnd(MyQueueId, &message,sizeof(message.proc), !IPC_NOWAIT);
            if (send_val == -1)    
            {
                fflush(stdout);
                perror("Errror in send To Scheduler\n");
            }
            //signal the scheduler to know that a process arrived and add it to the PCB
            kill(pidScheduler,SIGUSR1);
            down(semComm); //Sem to make sure the scheduler handled the last signal
        }
        
        
    }
    while(1);
    // 7. Clear clock resources
    destroyClk(true);
}

void clearResources(int signum)
{
    kill(pidScheduler,SIGINT); // send SIGINT to the scheduler to terminate
    msgctl(MyQueueId, IPC_RMID, (struct msqid_ds *)0);
    exit(0);
}

