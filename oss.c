//copy assignment 3
    //random number generator for forked processes (release and aquire new resources, update resource table)
    //message queue to communicate requests, allocation, and release of resources 
    //add deadlock detection and recovery algorithm (add fixed time to continuosly check this)

    //test in log file
/////////////////////////////////////////////////////////////////////////

// Name: Gabrielle Hieken
// Class: 4760 Operating Systems
// Date: 4/15/2023

#include <stdio.h>
#include <getopt.h> //Needed for optarg function
#include <stdlib.h> //EXIT_FAILURE
#include <unistd.h> //for pid_t and exec
#include <sys/types.h>
#include <time.h> //to create system time
#include <sys/shm.h> //Shared memory
#include <stdbool.h> //bool values
#include <sys/wait.h> //wait
#include <string.h> //remedy exit warning
#include <sys/msg.h> //message queues
#include "oss.h"

void printTable();

//My message queue initialization
struct msgqueue {
    long mtype;
    char mtext[200];
}msq;

int main(int argc, char *argv[]){
    //logfile declaration
    char* logFile = "logfile";

    //variables for our system clock
    struct timespec start, stop;
    double sec;
    double nano;
    int milliLim = 500; //time limit for random forked processes in milliseconds

    //for the file 
    FILE *fileLogging;

    //child process ID
    pid_t childpid = 0;

    //Parse through command line options
	char opt;
    while((opt = getopt(argc, argv, "hf:")) != -1 )
    {
        switch (opt)
        {
        //help message
        case 'h':
			printf("To run this project: \n\n");
            printf("run the command: ./oss -n num -s num -t num\n\n");
                    printf("\t-f = name of logfile you wish to write the process table in oss.c to\n\n"); 
                    printf("If you leave out '-f' in the command line prompt it will default to 'logfile'\n\n");
                    printf("Have fun :)\n\n");

                    exit(0);
            break;
        case 'f':
            logFile = optarg; 
            break;
        default:
            printf ("Invalid option %c \n", optopt);
            return (EXIT_FAILURE);
        }
    }

    //Initialize resource table
    int resourceTable[18][10];

    //Create empty resource table (all zeros)
    int i;
    int j;
    for(i = 0; i < 18; i++){
        for(j = 0; j < 10; j++){
            resourceTable[i][j] = 0;
        }
    }

    //Create resource header
    printf("\t");
    for(i=0;i<10;i++){
        printf("R%i\t", i);
    }
    printf("\n");

    //Print resource table and max processes on the side
    for(i = 0; i < 18; i++){
        printf("P%i\t", i);
        for(j = 0; j < 10; j++){
            printf("%i\t", resourceTable[i][j]);
        }
        printf("\n");
    }

    //Open the log file before input begins 
    fileLogging = fopen(logFile, "w+");

    //Create random millisecond between 1 - 500
    srand(time(0));


    //Create shared memory, key
    const int sh_key = 3147550;

    //Create key using ftok() for more uniqueness
    key_t msqkey;
    if((msqkey = ftok("oss.h", 'a')) == (key_t) -1){
        perror("IPC error: ftok");
        exit(1);
    }

    //create a new message queue
    int msqid;
    if ((msqid = msgget(msqkey, PERMS | IPC_CREAT)) == -1) {
      perror("Failed to create new private message queue");
      exit(1);
   }

    //create shared memory
    int shm_id = shmget(sh_key, sizeof(double), IPC_CREAT | 0666);
    if(shm_id <= 0) {
        fprintf(stderr,"ERROR: Failed to get shared memory, shared memory id = %i\n", shm_id);
        exit(1);
    }

    //attatch memory we allocated to our process and point pointer to it 
    double *shm_ptr = (double*) (shmat(shm_id, NULL, 0));
    if (shm_ptr <= 0) {
        fprintf(stderr,"Shared memory attach failed\n");
        exit(1);
    }

    //start the simulated system clock
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
      return EXIT_FAILURE;
    }

    //intialize values for use in while loop
    double currentTime;
    double limitReach = 0;
    double writeToMem;
    int numofchild = 0;

    //----------------------------------------------------------------------------------------------------------------------------------------
    //for forking the first child on the first loop
    int milliSec = 0;

    //Loop to handle our children processes and print the process table
    while(1) {
        printf("num of children %i\n", numofchild);
        //stop simulated system clock
        if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
            perror( "clock gettime" );
            return EXIT_FAILURE;
        }
        
        sec = (stop.tv_sec - start.tv_sec); 
        nano = (double)( stop.tv_nsec - start.tv_nsec);

        //if start time nanosecond is greater than stop, carry the one to get positive nanosecond
        if(start.tv_nsec > stop.tv_nsec){
            sec = (stop.tv_sec - start.tv_sec) - 1;
            nano = (double)( stop.tv_nsec - start.tv_nsec) + ((double)(1)*BILLION);
        }
        currentTime = sec + nano/BILLION;

        //Write the seconds and nanoseconds to memory for children to read
        writeToMem = currentTime;

        *shm_ptr = writeToMem;
        writeToMem = *shm_ptr;

        printf("wrote to mem: %lf\n", currentTime);
    
        if(limitReach <= currentTime){
            numofchild++;
           
            milliSec = randomNumberGenerator(milliLim); //create random number for next child to fork at 
            printf("random milliSecond: %i\n", milliSec);

            //combine seconds, milliseconds, and nanoseconds as one decimal to get new time to fork process
            limitReach = sec + (double)(milliSec/1000) + (double)(nano/BILLION);

            printf("we are making a new process at: %lf\n", limitReach); 
            printf("sec is %lf, mili  is %lf, nano is %lf\n", sec, (double)(milliSec/1000), (double)(nano/BILLION)); 

            childpid = fork(); //fork child

            if (childpid == -1) {
                perror("Failed to fork");
                return 1;
            }
            if (childpid == 0){  //send shared memory key to user_proc for children to use 
                char sh_key_string[50];
                snprintf(sh_key_string, sizeof(sh_key_string), "%i", sh_key);

                printf("shkey oss: %s\n", sh_key_string);

                char *args[] = {"user_proc", sh_key_string, NULL};
                //exec function to send children to user_proc along with our shared memory key
                execvp("./user_proc", args);

                return 1;
            }
            if(childpid != 0 ){ 
                // //initialize mtype to 1
                msq.mtype = 1;
                // char sec_string[50];
                // char nano_string[50];

                // //convert integer to char string
                // snprintf(sec_string, sizeof(sec_string), "%i", seconds);
                // snprintf(nano_string, sizeof(nano_string), "%i", nanoseconds);

                // //add seconds and nanoseconds together with a space in between to send as one message
                // char *together;
                // together = malloc(strlen(sec_string) + strlen(nano_string) + 1 + 1);
                // strcpy(together, sec_string);
                // strcat(together, " ");
                // strcat(together, nano_string);

                // //copy our new string into mtext
                // strcpy(msq.mtext, together);

                // //send our string to message queue
                // msgsnd(msqid, &msq, sizeof(msq), 0);
            }
        }

        // //recieve the message
    // msgrcv(msqid, &msq, sizeof(msq), 1, 0);
        if(numofchild > 0){
            break;
        }
    }  

    ///printf("deleting memory");
    shmdt( shm_ptr ); // Detach from the shared memory segment
    shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id 

    //Removes the message queue immediately
    if (msgctl(msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl");
            return EXIT_FAILURE;
    }

    //close the log file
    fclose(fileLogging);
    return 0;
}
