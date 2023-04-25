#include <stdio.h>
#include <unistd.h> //for pid_t and exec

#define BILLION 1000000000L
#define PERMS 0644

//20 instances of each resource 
int R0, R1, R2, R3, R4, R5, R6, R7, R8, R9 = 20;

struct sysTime{
int nano; // either true or false
int sec; // process id of this child
};
struct sysTime sysClock;

//Create random second and nanosecond in bound of user input
int randomNumberGenerator(int limit)
{
    int sec;

    sec = (rand() % (limit)) + 1;

    return sec;
}