/*
Name: Aditya Taday
ID number: 109550833

Using Semaphores to access shared memory between two
child processes and sending signals between processes

 */

#include <stdio.h>	     
#include <string.h>
#include <sys/types.h>   
#include <sys/ipc.h>     
#include <sys/shm.h>	 
#include <sys/sem.h>	 
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>	 
#include <wait.h>	 
#include <time.h>	 
#include <stdlib.h>	 
#include <fcntl.h>
#include <errno.h>

#define SEM_ID    250	          /* ID for the semaphore.        */
#define BUF_SIZE 1024
#define SHM_SIZE 2048             /* Shared memory size           */

union semun {                     /* semaphore value, for semctl(). */
    int val;
    struct semid_ds *buf;
    ushort * array;
};
char* monitor_table;     /* To store signals from producer-consumer */ 


/*
 * function to introduce a random delay between two operations.
 */
void random_delay()
{
    static int initialized = 0;
    int num;
    struct timespec delay;

    /* check whether random number generator is initialized, if not
       initialize with current time as seed
    */
    if (!initialized) {
		srand(time(NULL));
		initialized = 1;
    }

    num = rand() % 10;
    delay.tv_sec = 0;
    delay.tv_nsec = 10 * num;
    nanosleep(&delay, NULL);
}

/*
 * function to lock the semaphore, for exclusive access to a resource.
 */
void lock_semaphore(int semid)
{
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}

/*
 * function to unlock the semaphore, previously locked by lock_semaphore.
 */
void unlock_semaphore(int semid)
{
    struct sembuf sem_op;

    /* signal the semaphore - increase its value by one. */
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}

void consumer(pid_t pid, int semid, ssize_t* bytes_copied, char* buf, char* file)
{
  
    int fd;
    ssize_t nwrite, nread;

    fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        perror("consumer:");
        return;
    }

	do
    {
      	random_delay();        /* Wait for producer to write to shared buffer */
        lock_semaphore(semid); /* Lock the resource */ 
        nread = *bytes_copied;
        if(nread > 0) {
            nwrite = write(fd, buf, nread);
        	/* printf("\nconsumer write: %d bytes", nwrite); */
        	kill(pid, SIGUSR2);    /* Send signal to main process */
        }
        *bytes_copied = 0;
      	unlock_semaphore(semid);
    }
	while (nread >= 0);
  
  	close(fd);
    kill(pid, SIGUSR2);  /* After the file is copied, send signal to main */
    random_delay(); random_delay();
    kill(pid, SIGUSR2);  /* After the file is copied, send signal to main */
}

void producer(pid_t pid, int semid, ssize_t* bytes_copied, char* buf, char* file)
{
  
    int fd;
    ssize_t nread;

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("producer:");
        return;
    }

    do {
    	lock_semaphore(semid);   /* Lock the shared resource */
        nread = 1;
        if(*bytes_copied == 0) {
            nread = read(fd, buf, BUF_SIZE);
            if(nread > 0)
                *bytes_copied = nread;
            else
                *bytes_copied = -1;
            /* printf("\nproducer read: %d bytes", nread); */
        	kill(pid, SIGUSR1); /* Send signal to main process */
        }  
    	unlock_semaphore(semid);
		random_delay();     /* Wait for consumer to read from shared resource */
    }
    while(nread > 0);

    close(fd);

}

static void signal_handler(int signal_no)
{
    static int table_counter = 0;
    int c;

    /* which signal received? */
    switch( signal_no )
    {
        /* Signal is from producer */
        case SIGUSR1:
        	monitor_table[table_counter++] = 'P';
            break;
        
        /* Signal is from consumer */
        case SIGUSR2:
            if(table_counter > 0) {
                if(monitor_table[table_counter - 1] == 'C') {
                    /* Two consecutive signals from Consumer indicates end. */
                    printf("\nMonitor Table Output\n");
                    for(c = 0; c < table_counter;c++) {
                        printf("%c ", monitor_table[c]);
                    }
                    printf("C\n");
                }
            }        
        	monitor_table[table_counter++] = 'C';
            break;

        default:
            break;
    }

    return;
}

int main(int argc, char* argv[])
{
    int semid, shmid, retval; /* Semaphore ID and Shared memory segment ID */
    char* shm_addr; 	      /* address of shared memory segment.  */
    union semun sem_val;    
    ssize_t* bytes_copied;     
    char* buf;             
    struct shmid_ds shm_desc;
	struct stat st;
    pid_t chpid1, chpid2, pid_main;  /* PID of parent and child processes.  */
    char file1[255], file2[255];

    if(argc >= 3) {
        strcpy(file1, argv[1]);   /* file input from command line */
        strcpy(file2, argv[2]);   /* file output from command line */
    }
    else if(argc == 2) {
        strcpy(file1, argv[1]);   /* file input from command line */
        strcpy(file2, "file2");   /* default output file */
    }  
    else {
        strcpy(file1, "file1");   /* default values for input and output */
        strcpy(file2, "file2");
    }

    if (stat(file1, &st) == -1) {
		perror("stat:");
		exit(EXIT_FAILURE);
	}
    if(st.st_size > 2000000) {
        printf("File size is too big for this process : %ld bytes!\n", st.st_size);
        exit(EXIT_FAILURE);
    }
    monitor_table = (char*)malloc( (st.st_size / 512) + 10);

    /* Create the two user-defined signal handlers */ 
    if( signal( SIGUSR1, signal_handler) == SIG_ERR  )
    {
        perror("main: ");
    }

    if( signal( SIGUSR2, signal_handler) == SIG_ERR  )
    {
        perror("main: ");
    }
  	pid_main = getpid(); /* Get process id of parent */
 
    /* create a semaphore set with one semaphore with owner access only   */
    semid = semget(SEM_ID, 1, IPC_CREAT | 0600);
    if (semid == -1) {
		perror("main: semget");
		exit(1);
    }

    /* intialize the first semaphore to '1'. */
    sem_val.val = 1;
    retval = semctl(semid, 0, SETVAL, sem_val);
    if (retval == -1) {
		perror("main: semctl");
		exit(1);
    }

    /* allocate a shared memory segment with the predefined size in bytes. */
    shmid = shmget(100, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0600);
    if (shmid == -1) {
        perror("main: shmget: ");
        exit(1);
    }

    /* attach the shared memory segment to our process's address space. */
    shm_addr = shmat(shmid, NULL, 0);
    if (!shm_addr) { /* error */
        perror("main: shmat: ");
        exit(1);
    }

    /* Assign address of elements in shared memory space. */
    bytes_copied = (ssize_t*) shm_addr;
    *bytes_copied = 0;
    buf = (char*) ((void*)shm_addr+sizeof(ssize_t));

    /* fork child processes to trasnfer the file. */
    chpid1 = fork();
    if (chpid1 == 0) {
	    producer(pid_main, semid, bytes_copied, buf, file1);
	    exit(0);
    }
    chpid2 = fork();
    if (chpid2 == 0) {
	    consumer(pid_main, semid, bytes_copied, buf, file2);
	    exit(0);
    }

    /* wait for child process's termination. */
    {
        int child_status;

        //wait(&child_status);
        //wait(&child_status);
		while( (waitpid(chpid1, &child_status, 0 ) == -1) && (errno == EINTR) ) {}
		while( (waitpid(chpid2, &child_status, 0 ) == -1) && (errno == EINTR) ) {}
    }

    /* detach the shared memory segment from our process's address space. */
    if (shmdt(shm_addr) == -1) {
        perror("main: shmdt: ");
    }

    /* de-allocate the shared memory segment. */
    if (shmctl(shmid, IPC_RMID, &shm_desc) == -1) {
        perror("main: shmctl: ");
    }

    free(monitor_table);
    return 0;
}