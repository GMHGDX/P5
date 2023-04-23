#include <stdio.h>
#include <unistd.h> //for pid_t and exec

#define BILLION 1000000000L
#define PERMS 0644

struct RT {
int resource; // either true or false
char process; // process id of this child
};
struct RT resourceTable[20];