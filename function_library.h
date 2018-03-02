#include<unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#ifndef FUNCTION_LIBRARY_H
#define FUNCTION_LIBRARY_H

int checkNumber(const char *);
void writeError(const char *, const char *);
pid_t createChildProcess(const char *, const char *);
int makeargv(const char *, const char *, char***);
key_t getKey(int);
void* getExistingSharedMemory(int, const char *);
void deallocateSharedMemory(int, const char *);
int setPeriodic(double);
//void process(const int, const int, int*, int*);
#endif
