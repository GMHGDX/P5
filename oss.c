//copy assignment
    //get 2 or more children to run at once
    //add a blocked queue for when there are not enough resources to request 
    //add deadlock detection and recovery algorithm (add fixed time to continuosly check this)
    //add input for log file, test logfile
    
/////////////////////////////////////////////////////////////////////////

// Name: Gabrielle Hieken
// Class: 4760 Operating Systemsd
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

//msgbuffer for message queue
typedef struct queue {
    int pid;
    int resources[10];
} queue;

#define MAX 40

queue blockedQueue[MAX];
int front = 0;
int rear = -1;
int itemCount = 0;

queue peek() { return blockedQueue[front]; }

bool isEmpty() { return itemCount == 0; }

bool isFull() { return itemCount == MAX; }

int size() { return itemCount; }  

void insert(queue data) {
   if(!isFull()) {
      if(rear == MAX-1) {
        rear = -1;            
      }
      rear++;
      int i;
      for(i=0;i<10;i++){
        blockedQueue[rear].resources[i] = data.resources[i];
      }
      blockedQueue[rear].pid = data.pid;
      itemCount++;
   }
}

queue removeData() {
   queue data = blockedQueue[front++];
   if(front == MAX) {
      front = 0;
   }
   itemCount--;
   return data;  
}

void printResourcetable(int resourceTable[][10]);

typedef struct pidstruct {
    pid_t realpid;
    int simpid;
} pidstruct;

int main(int argc, char *argv[]){
    int i;
    int j;

    queue toInsert; //insert numbers to blocked queue struct
    char* logFile = "logfile"; //logfile declaration
    FILE *fileLogging; //for the file 
    pid_t childpid = 0; //child process ID 
    int resourceTable[18][10]; //Initialize resource table
    int resourcesLeft[10];
    pidstruct mypidstruct[50];

    for(i=0;i<50;i++){
        mypidstruct[i].simpid = -1;
    }

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

    //Initialize empty resources left (all zeros)
    for(i = 0; i < 10; i++){
        resourcesLeft[i] = 20;
    }

    //Initialize empty resource table (all zeros)
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
    int milliSec = 0; //milliseconds used in time limit
    int resourcesUsed[10]; //resources in an array
    char* text; //used to seperate message recieved by whitespace 
    int simpidofsender;
    bool notenoughresources = false;
    int checkWhatToDo = -1;
    bool allResourcesFree = false;

    //Loop to handle our children processes and print the process table ---------------------------------------------------------------------
    while(1) {
        //printf("num of children %i\n", numofchild); //TESTING

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

        //  i = 0;  //Finds the smallest positoin to put the new child into (cannot be over 17 or it will break our code)
        //  while(mypidstruct[i].simpid != 0){
        //       i++;
        //  }
        // if(limitReach <= currentTime && numofchild < 40 && currentTime <= 5 && i < 17) // if its time to create a child (limitReach <= currentTime), and we ahve less then 40 children, and 5 seconds have not passsed, and we have sapce for 1 more child (i < 17)
        if(numofchild < 2){   //For testing //add parameter for 40 children and more than 5 real seconds
            numofchild++;
           
            milliSec = randomNumberGenerator(milliLim); //create random number for next child to fork at 
            //printf("random milliSecond: %i\n", milliSec); //TESTING

            limitReach = sec + (double)(milliSec/1000) + (double)(nano/BILLION); //combine sec, millisec, and nanosec as one decimal to get new time to fork process

            //printf("we are making a new process at: %lf\n", limitReach);  //TESTING
            //printf("sec is %lf, mili  is %lf, nano is %lf\n", sec, (double)(milliSec/1000), (double)(nano/BILLION)); // TESTING 

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
                i = 0;  //Finds the smallest positoin to put the new child into (cannot be over 17 or it will break our code)
                while(mypidstruct[i].simpid != -1){
                    i++;
                }
                if(i>17){
                    printf("fuck i broke the code");
                }
                mypidstruct[i].simpid = i;
                mypidstruct[i].realpid = childpid;

                // for(i=0;i<10;i++){
                //     printf("mypidstruct %i realpid %i\n", i, mypidstruct[i].realpid);
                // }
            }
        }

        buf.intData = 0;
        strcpy(buf.strData, "-1"); //Clear the message string back to nothing before we check for a msgrcv
        checkWhatToDo = -1; //Return checkwaht todo back to "do nothing"

        // receive a message from user_proc, but only one for our PID
        msgrcv(msqid, &buf, sizeof(msgbuffer), getpid(), IPC_NOWAIT);
        //printf("OSS checking message--> msg: %s intdata: %d and mtype %f\n", buf.strData, buf.intData, buf.mtype); //TESTING
        checkWhatToDo = atoi(buf.strData);  //If 0, means a process has died, if greater than 0, meana we got some reacourses to alloacte
        //printf("checlkwhat to do is %i\n", checkWhatToDo);

        if(checkWhatToDo == 0){
            //de allocate ur shit
            printf("dealloacting\n");
            i = 0;
            printf("sender is: %i\n", buf.intData);
            for(i=0;i<5;i++){
                printf("mypidstruct %i realpid %i\n", i, mypidstruct[i].realpid);
            }
            i = 0;
            while(i < 18){
                printf("I is %i\n", i);
                if(mypidstruct[i].realpid == buf.intData){  //Will this crash if mypidstruct[i].realpid is not set to anything (unitalized)
                    simpidofsender = mypidstruct[i].simpid;
                    printf("found pid %i in position %i, with simpid %i\n", mypidstruct[i].realpid, i, mypidstruct[i].simpid);

                    mypidstruct[i].realpid = 0; //Clear out the position in mypidstruct for reuse
                    mypidstruct[i].simpid = -1;

                    printf("recources deallocated: ");
                    //Update resource table with new values
                    for (i=0;i<10;i++){
                        printf(" %i",  resourceTable[simpidofsender][i]);
                        resourcesLeft[i] += resourceTable[simpidofsender][i];
                        resourceTable[simpidofsender][i] = 0;
                    }
                    printf("\n");


                    //break;
                    i = 20;
                }
                i++;
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

            // printf("recources deallocated: ");
            // //Update resource table with new values
            // for (i=0;i<10;i++){
            //     printf(" %i",  resourceTable[simpidofsender][i]);
            //     resourcesLeft[i] += resourceTable[simpidofsender][i];
            //     resourceTable[simpidofsender][i] = 0;
            // }
            
        }
        if(checkWhatToDo > 0){
            printf("OSS recieved--> resources: %s my int data(child is): %d and mtype %ld\n", buf.strData, buf.intData, buf.mtype); //TESTING

            text = strtok(buf.strData, " ");
            for (i=0;i<10;i++){
                if(text == NULL){
                    break;
                }
                resourcesUsed[i] = atoi(text);
                text = strtok(NULL, " ");
            }

            i = 0;
            while(i < 18){
                if(mypidstruct[i].realpid == buf.intData){  //Will thius crash if mypidstruct[i].realpid is not set to anything (unitalized)
                    simpidofsender = mypidstruct[i].simpid;
                    break;
                }
                i++;
            }

            printf("the simulated pid of the sender is: %i\n", simpidofsender);

            //Check if we have enough resources for this process
            for(i=0;i<10;i++){
                if((resourcesLeft[i] - resourcesUsed[i]) < 0){
                    notenoughresources = true;
                }
            }

            if(!notenoughresources){
                //send message back to child that there are enough resources
                strcpy(buf.strData, "1");
                buf.mtype = buf.intData;
                printf("OSS is sending that it has available resources--> message: %s my int data(child is): %d\n", buf.strData, buf.intData); //TESTING
                if (msgsnd(msqid, &buf, sizeof(msgbuffer)-sizeof(long), 0) == -1) { perror("msgsnd to child 1 failed\n"); exit(1); } 

                //Update resource table with new values
                for (i=0;i<10;i++){
                    resourceTable[simpidofsender][i] = resourcesUsed[i];
                    resourcesLeft[i] -= resourcesUsed[i];
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
            }else{  //If notenough is true
                //send to blocked queue, should hold the pid of the process that is blocked and the rescoruces it is reuqesating, first in first out
                printf("Not enough resources! Get blocked! \n");
                toInsert.pid = buf.intData;
                for(i=0;i<10;i++){
                    toInsert.resources[i] = resourcesUsed[i];
                }
                insert(toInsert);

                //TESTING
                toInsert = peek();
                printf("Test: looking at front of blocekd queue pid = %i\n Resources are:", toInsert.pid);
                for(i=0;i<10;i++){
                    printf(" %i", toInsert.resources[i]);
                }
                printf("\n");
            } 
            notenoughresources = false;
        }
        //End of msgrcv


        if(!isEmpty()){ //Is blocekd queue empty
            //printf("checking if there is sapce for my fucko\n");
            //Check if the front of the blocked queue has enough resoucres to be allowed to run
            toInsert = peek();
            for (i=0;i<10;i++){
                resourcesUsed[i] = toInsert.resources[i];
            }
            //Check if we have enough resources for this process
            // printf("Remainder:");
            // for(i=0;i<10;i++){
            //     printf(" %i", (resourcesLeft[i] - resourcesUsed[i]));
            //     if((resourcesLeft[i] - resourcesUsed[i]) < 0){
            //         printf("do not ahve enough\n");
            //         notenoughresources = true;
            //     }
            // }
            // printf("\n");

            // printf("RescouresLeft:");
            // for(i=0;i<10;i++){
            //     printf(" %i", resourcesLeft[i]);
            // }
            // printf("\n");

            // printf("Rescouresused:");
            // for(i=0;i<10;i++){
            //     printf(" %i", resourcesUsed[i]);
            // }
            // printf("\n");


            if(!notenoughresources){
                printf("have enough!!!!!!!\n");
                removeData(); //Delete recourse from front of queue
                //send message back to child that there are enough resources
                strcpy(buf.strData, "1");
                buf.mtype = toInsert.pid;
                buf.intData = toInsert.pid;
                printf("OSS is sending that it has available resources--> message: %s my int data(child is): %d\n", buf.strData, buf.intData); //TESTING
                if (msgsnd(msqid, &buf, sizeof(msgbuffer)-sizeof(long), 0) == -1) { perror("msgsnd to child 1 failed\n"); exit(1); } 

                //Update resource table with new values
                for (i=0;i<10;i++){
                    resourceTable[simpidofsender][i] = resourcesUsed[i];
                    resourcesLeft[i] -= resourcesUsed[i];
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
            }
            // else{
            //     prtinf("still cant get the guy out from bloeck queue");
            // }
            notenoughresources = false;
        }

        //printf("checking if its time to end");
        //Check if we should terminate porgram
        //printf("resourcesLeft::: ");
        for(i=0;i<10;i++){
            //printf(" %i", resourcesLeft[i]);
            if(resourcesLeft[i] != 20){
                //printf("Not :( set\n");
                allResourcesFree = false;
                break; //Get out of this for loop, there are programs still using recources
            }
            if(i == 9 && resourcesLeft[i] == 20){
                //printf("perfectly set\n");
                allResourcesFree = true;
            }
        }

       //printf("%d && %d && %d && %d\n", allResourcesFree, isEmpty(), numofchild>1 , currentTime > 5);
        if(allResourcesFree && isEmpty() && numofchild>1 && currentTime > 5){
            printf("All paraemtersa are met. ENDING____");
            break; //end program
        }else{
            allResourcesFree = false;
        }
        

        //end of loop
    }  

    printf("waiting for the child to end its own life\n");
    wait(0); //wait for child to finish in user_proc

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

    printf("RescouresLeft:");
    for(i=0;i<10;i++){
        printf(" %i", resourcesLeft[i]);
    }
    printf("\n");

    // printf("MyPidStruct:\n");
    // for(i=0;i<20;i++){
    //     printf("%i:  realid: %i, simid: %i\n", i, mypidstruct[i].realpid, mypidstruct[i].simpid);
    // }
    // printf("\n");

    //printf("blockedQueue:");

    printf("deleting memory");
    shmdt( shm_ptr ); // Detach from the shared memory segment
    shmctl( shm_id, IPC_RMID, NULL ); // Free shared memory segment shm_id 

    //Removes the message queue immediately
    if (msgctl(msqid, IPC_RMID, NULL) == -1) { perror("msgctl"); return EXIT_FAILURE; }

    fclose(fileLogging); //close the log file

    return 0;
}
