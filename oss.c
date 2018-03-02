// oss.c
// Code for master executable which will be used to spawn producer and consumer children as well as handle
// signals
// Alex Kane
// 3/1/18
// CS 4760 Project 3
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/shm.h>
#include<sys/ipc.h>
#include<sys/types.h>
#include<signal.h>
#include<time.h>
#include"function_library.h"
#include"oss_constants.h"

// Function declarations
void signalInterruption(int);
void deallocateAllSharedMemory(char*);

// Keep track of shmids globally because signal handlers
// will also need to use them
int shmidSeconds;
int shmidNanoSeconds;

int main(int argc, char** argv)
{
	// Set signal handler for signals which may interrupt the process
	// including our periodic timer and keyboard interruption
	signal(SIGINT, signalInterruption);
	signal(SIGALRM, signalInterruption);

	// allocate shared memory and get shmids
	shmidSeconds = allocateSharedMemory(INT_SECONDS_KEY, argv[0]);
	shmidNanoSeconds = allocateSharedMemory(INT_NANO_SECONDS_KEY, argv[0]);
	
	setPeriodic(NUM_SECONDS_WAIT);

	deallocateAllSharedMemory(argv[0]);
	return 0;
}

void deallocateAllSharedMemory(char* processName)
{
	deallocateSharedMemory(shmidSeconds, processName);
	deallocateSharedMemory(shmidNanoSeconds, processName);
}

void signalInterruption(int signo)
{

}
