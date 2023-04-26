//copy assignment 3
    //release and aquire new resources, update resource table
    //add a sleeping queue for when there are not enough resources to request 
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

typedef struct pidstruct {
    pid_t realpid;
    int simpid;
} pidstruct;



int main(int argc, char *argv[]){
    pidstruct mypidstruct[50];

    char* logFile = "logfile"; //logfile declaration
    FILE *fileLogging; //for the file 
    pid_t childpid = 0; //child process ID 
    int resourceTable[18][10]; //Initialize resource table
    srand(time(0)); //Seed the random number generator

    //variables for our system clock
    struct timespec start, stop;
    double sec;
    double nano;
    int milliLim = 500; //time limit for random forked processes in milliseconds

    //Create shared memory key and message buffer for message queue
    const int sh_key = 3147550;     
    key_t msqkey;
    int msqid;
    msgbuffer buf;

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

    //Create empty resource table (all zeros)
    int i;
    int j;
    for(i = 0; i < 18; i++){
        for(j = 0; j < 10; j++){
            resourceTable[i][j] = 0;
        }
    }

    printResourceTable(resourceTable); //print resource table
    
    fileLogging = fopen(logFile, "w+"); //Open the log file before input begins 

    //Create message queue
    if((msqkey = ftok("oss.h", 'a')) == (key_t) -1){ perror("IPC error: ftok"); exit(1); } //Create key using ftok() for more uniquenes
    if ((msqid = msgget(msqkey, PERMS | IPC_CREAT)) == -1) { perror("Failed to create new private message queue"); exit(1); } //open an existing message queue or create a new one

    //create shared memory
    int shm_id = shmget(sh_key, sizeof(double), IPC_CREAT | 0666);
    if(shm_id <= 0) {fprintf(stderr,"ERROR: Failed to get shared memory, shared memory id = %i\n", shm_id); exit(1); }

    //attatch memory we allocated to our process and point pointer to it 
    double *shm_ptr = (double*) (shmat(shm_id, NULL, 0));
    if (shm_ptr <= 0) { fprintf(stderr,"Shared memory attach failed\n"); exit(1); }

    //start the simulated system clock
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) { perror( "clock gettime" ); return EXIT_FAILURE; }

    //intialize values for use in while loop
    double currentTime; //time going into shared memory
    double limitReach = 0; //random time next child is forked 
    double writeToMem;
    int numofchild = 0; //DELETEEEEEEEEE
    char msgForChild[10]; //char 0for converting int sent to user_proc
    int milliSec = 0; //milliseconds used in time limit
    int reasourcesUsed[10]; //resources in an array
    char* text; //used to seperate message recieved by whitespace 
    int simpidofsender;

    //Loop to handle our children processes and print the process table ---------------------------------------------------------------------
    while(1) {
        printf("num of children %i\n", numofchild); //TESTING

        //stop simulated system clock and get seconds and nanoseconds
        if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) { perror( "clock gettime" ); return EXIT_FAILURE; }
        sec = (stop.tv_sec - start.tv_sec); 
        nano = (double)( stop.tv_nsec - start.tv_nsec);

        //if start time nanosecond is greater than stop, carry the one to get positive nanosecond
        if(start.tv_nsec > stop.tv_nsec){
            sec = (stop.tv_sec - start.tv_sec) - 1;
            nano = (double)( stop.tv_nsec - start.tv_nsec) + ((double)(1)*BILLION);
        }
        currentTime = sec + nano/BILLION;

        //Write the current time to memory for children to read
        writeToMem = currentTime;
        *shm_ptr = writeToMem;
        //writeToMem = *shm_ptr;//TESTING (reading from share memory)

        printf("wrote to mem: %lf\n", currentTime); //TESTING
    
        if(limitReach <= currentTime){ //fork child if current time is more than random time to fork child
            numofchild++;
           
            milliSec = randomNumberGenerator(milliLim); //create random number for next child to fork at 
            printf("random milliSecond: %i\n", milliSec); //TESTING

            limitReach = sec + (double)(milliSec/1000) + (double)(nano/BILLION); //combine sec, millisec, and nanosec as one decimal to get new time to fork process

            printf("we are making a new process at: %lf\n", limitReach);  //TESTING
            printf("sec is %lf, mili  is %lf, nano is %lf\n", sec, (double)(milliSec/1000), (double)(nano/BILLION)); // TESTING 

            childpid = fork(); //fork child

            if (childpid == -1) { perror("Failed to fork"); return 1; }
            if (childpid == 0){  //send shared memory key to user_proc for children to use 
                char sh_key_string[50];
                snprintf(sh_key_string, sizeof(sh_key_string), "%i", sh_key);

                char *args[] = {"user_proc", sh_key_string, NULL};
                execvp("./user_proc", args); //exec function to send children to user_proc along with our shared memory key

                return 1;
            }
            if(childpid != 0 ){ 
                mypidstruct[numofchild].realpid = childpid;
                mypidstruct[numofchild].simpid = numofchild-1;

                buf.mtype = childpid; //initialize mtype to the child's pid
                buf.intData = childpid; //we will give it the pid we are sending to, so we know it received it

                snprintf(msgForChild, sizeof(msgForChild), "%i", childpid); //convert int message to string
                printf("Sending message to child: %s with pid %d \n", msgForChild, childpid); //TESTING

                strcpy(buf.strData, msgForChild); //copy msg contents into the buffer
                
                //send message to user_proc
                if (msgsnd(msqid, &buf, sizeof(msgbuffer)-sizeof(long), 0) == -1) { perror("msgsnd to child 1 failed\n"); exit(1); } 
            }
        }

        wait(0); //wait for child to finish in user_proc

        buf.intData = 0;
        // receive a message from user_proc, but only one for our PID
        if (msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), IPC_NOWAIT) == -1) { perror("failed to receive message from parent\n"); exit(1); }
        if(buf.intData != 0){
            printf("OSS recieved--> resources: %s my int data(child is): %d\n", buf.strData, buf.intData); //TESTING

            text = strtok(buf.strData, " ");
            for (i=0;i<10;i++){
                if(text == NULL){
                    break;
                }
                reasourcesUsed[i] = atoi(text);
                text = strtok(NULL, " ");
            }
            
            // printf("We parsed out:"); //TESTING
            // for (i=0;i<10;i++){
            //     printf(" %i", reasourcesUsed[i]);
            // }

            i = 0;
            while(i < 18){
                if(mypidstruct[i].realpid == buf.intData){  //Will thius crash if mypidstruct[i].realpid is not set to anything (unitalized)
                    simpidofsender = mypidstruct[i].simpid;
                    break;
                }
                i++;
            }

            printf("the simulated pid of the sender is: %i\n", simpidofsender);

            //Update resource table with new values
            for (i=0;i<10;i++){
                resourceTable[simpidofsender][i] = reasourcesUsed[i];
            }

            printResourceTable(resourceTable);
            
        }else{
            printf("No message received\n");
        }

        if(numofchild > 0){ //TESTING
            break;
        }
    }  

    ///printf("deleting memory");
    shmdt( shm_ptr ); // Detach from the shared memory segment
    shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id 

    //Removes the message queue immediately
    if (msgctl(msqid, IPC_RMID, NULL) == -1) { perror("msgctl"); return EXIT_FAILURE; }

    fclose(fileLogging); //close the log file

    return 0;
}

void printResourcetable(int resourceTable[]){
    int i, j;
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
}
