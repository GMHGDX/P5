#include <stdio.h>
#include <unistd.h> //for pid_t and exec

#define BILLION 1000000000L
#define PERMS 0644

struct sysTime{
int nano; // either true or false
int sec; // process id of this child
};
struct sysTime sysClock;