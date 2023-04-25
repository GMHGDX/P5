#include <stdio.h>
#include <string.h> //remedy exit warning
#include <stdlib.h> //EXIT_FAILURE
#include <sys/shm.h> //Shared memory
#include <sys/msg.h> //message queues
#include "oss.h"

//for message queue
struct msgqueue {
    long mtype;
    char mtext[200];
}msq;

int main(int argc, char *argv[]){
    int resourceLim = 20;
    int resourceAsk[10];

    srand(time(0));

    //grab sh_key from oss for shared memory
    int sh_key = atoi(argv[1]);

    //get shared memory
    int shm_id = shmget(sh_key, sizeof(double), 0666);
    if(shm_id <= 0) {
        fprintf(stderr,"CHILD ERROR: Failed to get shared memory, shared memory id = %i\n", shm_id);
        exit(1);
    }

    //attatch memory we allocated to our process and point pointer to it 
    double *shm_ptr = (double*) (shmat(shm_id, 0, 0));
    if (shm_ptr <= 0) {
        fprintf(stderr,"Child Shared memory attach failed\n");
        exit(1);
    }

    //read time from memory
    double readFromMem;
    readFromMem = *shm_ptr;

    printf("\nThis is the number child read from memory: %lf", readFromMem.currentTime);

   // --------------------------------------------------------------------------------//

    // //Create key using ftok() for more uniqueness
    // key_t msqkey;
    // if((msqkey = ftok("oss.h", 'a')) == (key_t) -1){
    //     perror("IPC error: ftok");
    //     exit(1);
    // }

    // //open an existing message queue or create a new one
    // int msqid;
    // if ((msqid = msgget(msqkey, PERMS | IPC_CREAT)) == -1) {
    //   perror("Failed to create new private message queue");
    //   exit(1);
    // }

    int i;
    for(i=0;i<10;i++){
        resourceAsk[i] = randomNumberGenerator(resourceLim);
    }

     printf("I need: ");
    for(i=0;i<10;i++){
        printf("%i ", resourceAsk[i]);
    }
    printf("\n");

    //concatenate all the resources requested by the process in one string
    char *together;
    char resourceAsk_string[50];
    together = malloc(strlen(resourceAsk_string)*10 + 1 + 1);
    

    for(i=0;i<10;i++){
        snprintf(resourceAsk_string, sizeof(resourceAsk_string), "%i", resourceAsk[i]); //convert integer to char string 
        if(i == 0){
            strcpy(together, resourceAsk_string);
        }else{
            strcat(together, resourceAsk_string);
        }
        if(i < 9){
            strcat(together, " ");
        }
         printf("added %s String is now %s\n", resourceAsk_string, together);
    }

    printf("These are your resources in one string format: %s\n", together); //testing

    // //copy our new string into mtext
    // strcpy(msq.mtext, resourceAsk);

    // //send our string to message queue
    // msgsnd(msqid, &msq, sizeof(msq), 0);

    return 0;

//--------------------------------------------------------------------------------------------------//

//     int termTimeS;
//     int termTimeNano;
//     int sysClockS;
//     int sysClockNano;
//     int checkSec = 0;

//     // initialization for string loop
//     int seperate = 0;
//     int sec;
//     int nanosec;



//     //attatch memory we allocated to our process and point pointer to it 
//     double *shm_ptr = (double*) (shmat(shm_id, 0, 0));
//     if (shm_ptr <= 0) {
//         fprintf(stderr,"Child Shared memory attach failed\n");
//         exit(1);
//     }

//     //read time from memory
//     double readFromMem;
//     readFromMem = *shm_ptr;

//     //Figure out when to terminate
//     termTimeS = readFromMem.sec + sec;
//     termTimeNano = readFromMem.nano + nanosec;

//     //Read time from memory
//     sysClockS = readFromMem.sec;
//     sysClockNano = readFromMem.nano;

//     //for printing every second in the loop for each child that starts at different times
//     checkSec = sysClockS;

//     printf("WORKER PID: %ld PPID: %ld Received message from oss: SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n--Received message\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

//     //loop child until termination time is passed 
//     while(1){
//         readFromMem = *shm_ptr;
//         sysClockS = readFromMem.sec;
//         sysClockNano = readFromMem.nano;

//         //to terminate at the right time
//         if(sysClockS > termTimeS || (sysClockS == termTimeS && sysClockNano > termTimeNano)){
//             break;
//         }
//         //printins out every second
//         if(checkSec == sysClockS){
//             printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --%i seconds has passed\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano, checkSec);
//             checkSec++;
//         }
//     }
//     //print when child has finished loop and terminating
//     printf("WORKER PID: %ld PPID: %ld SysClockS: %i SysclockNano: %i TermTimeS: %i TermTimeNano: %i\n --Terminating\n",(long)getpid(), (long)getppid(), sysClockS, sysClockNano, termTimeS, termTimeNano);

    return 0;
}