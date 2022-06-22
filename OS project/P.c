#include "headers.h"

int remainingtime;
int shmidProcessWithScheduler;
int sem1;
int sem2;
union Semun semun;
void terminate(int signum);
int MyID;
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

int main(int agrc, char * argv[])
{
    MyID= atoi(argv[1]);
    signal(SIGINT, terminate);
    key_t key_id1= ftok("Firstfile", getpid()); 
    key_t key_id2= ftok("Secondfile", getpid()); 
    int sem1 = semget(key_id1, 1, 0666 | IPC_CREAT);
    int sem2 = semget(key_id2, 1, 0666 | IPC_CREAT);
    if (sem1 == -1 || sem2 == -1)
    {
        fflush(stdout);
        perror("Process:Error in create sem");
        exit(-1);
    }
    semun.val = 1; /* initial value of the semaphore, Binary semaphore */
    if (semctl(sem1, 0, SETVAL, semun) == -1) // set initial value of sem 0 in the sem1 set = semun.val
    {
        fflush(stdout);
        perror("Process:Error in semctl");
        exit(-1);
    }
    if (semctl(sem2, 0, SETVAL, semun) == -1)
    {
        fflush(stdout);
        perror("Process:Error in semctl");
        exit(-1);
    }
    shmidProcessWithScheduler = shmget(getpid(), 256, IPC_CREAT | 0644);

    if (shmidProcessWithScheduler == -1)
    {
        fflush(stdout);
        perror("Process:Error in create shm");
        exit(-1);
    }
    void *shmAddr = shmat(shmidProcessWithScheduler, (void *)0, 0);
    if ((long)shmAddr == -1)
    {
        fflush(stdout);
        perror("Error in attach in writer");
        exit(-1);
    }
    char remainingtimeChar[10];
    
    up(sem1);
    strcpy(remainingtimeChar,(char *) shmAddr); 
    down(sem2); //When he wants to modify it
    remainingtime=atoi(remainingtimeChar);
    fflush(stdout);
    printf("remain %d\n",remainingtime);
    initClk();
    
    while (remainingtime > 0)
    {
        up(sem1);
        strcpy(remainingtimeChar,(char *) shmAddr); 
        down(sem2); //When he wants to modify it
        remainingtime=atoi(remainingtimeChar);
        fflush(stdout);
        printf("remain %d\n",remainingtime);
    }
    destroyClk(false);
    //Clear resources in case it terminates
    shmctl(shmidProcessWithScheduler, IPC_RMID, (struct shmid_ds *)0);
    semctl(sem1, 0, IPC_RMID, semun);
    semctl(sem2, 0, IPC_RMID, semun);
    exit(MyID);
    return 0;
}

void terminate(int signum)
{
    //Clear resources in case of interruption
    shmctl(shmidProcessWithScheduler, IPC_RMID, (struct shmid_ds *)0);
    semctl(sem1, 0, IPC_RMID, semun);
    semctl(sem2, 0, IPC_RMID, semun);
    exit(MyID);
}