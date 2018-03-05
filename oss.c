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
#include<sys/msg.h>
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

// Keep track of msgid so they can be easily deallocated
int msgIdCriticalSection;
int msgIdSend;

char* processName;

int main(int argc, char** argv)
{
	processName = argv[0];

#ifdef DEBUG_OUTPUT
	printf("Debugging output enabled\n");
#endif

	// Set signal handler for signals which may interrupt the process
	// including our periodic timer and keyboard interruption
	signal(SIGINT, signalInterruption);
	signal(SIGALRM, signalInterruption);

	// allocate shared memory and get shmids
	shmidSeconds = allocateSharedMemory(INT_SECONDS_KEY, argv[0]);
	shmidNanoSeconds = allocateSharedMemory(INT_NANO_SECONDS_KEY, argv[0]);

	// allocate message queues
	msgIdCriticalSection = allocateMessageQueue(INT_CRITICAL_MESSAGE, argv[0]);
	msgIdSend = allocateMessageQueue(INT_COMMUNICATION_MESSAGE, argv[0]);

	/*
	struct msgbuf buf;
	buf.mtype = 1;
	msgsnd(msgIdCriticalSection, &buf, sizeof(buf), 0);
	*/

	// map to the newly allocated variables
	int* seconds = shmat(shmidSeconds, 0, 0);
	int* nanoSeconds = shmat(shmidNanoSeconds, 0, 0);
	*seconds = 0;
	*nanoSeconds = 0;

#ifdef DEBUG_OUTPUT
	printf("SHMID of seconds: %d\n", shmidSeconds);
	printf("SHMID of nanoseconds: %d\n", shmidNanoSeconds);
#endif
	
	int argFlagHelp = 0, argFlagFileName = 0, numChildren = 5, timeToTerminate = 20;
	char fileName[FILENAME_LENGTH] = "oss.log";
	int c;
	while ((c = getopt(argc, argv, "hs:l:t:")) != -1)
	{
		switch (c)
		{
			case 'h':
				argFlagHelp = 1;
				displayHelp();
				break;
			case 's':
				if (optarg != NULL && checkNumber(optarg))
				{
					numChildren = atoi(optarg);
				}
				break;
			case 'l':
				if (optarg != NULL)
				{
					argFlagFileName = 1;
					strcpy(fileName, optarg);
				}
			case 't':
				if (optarg != NULL && checkNumber(optarg))
				{
					timeToTerminate = atoi(optarg);
				}	
			default:
				break;
		}
	}

#ifdef DEBUG_OUTPUT	
	printf("Help flag: %d\n", argFlagHelp);
	printf("Filename from command line: %s\n",fileName);
	printf("Number of slave processes from command line: %d\n", numChildren);
	printf("Amount of time to run before terminating: %d\n", timeToTerminate);
#endif

	setPeriodic(timeToTerminate);

	// if passed in a help flag, we don't want to do anything else - just exit
	if (argFlagHelp)
	{
		return 0;
	}
	
	// spawn off child processes and allocate variables for chile tracking
	pid_t* childPids = malloc(sizeof(pid_t) * numChildren);
	int i, numProcesses = 0, totalProcessesCreated;
	for (i = 0; i < numChildren; ++i)
	{
		childPids[i] = createChildProcess("./user", argv[0]);
		++numProcesses;
	}

	totalProcessesCreated = numProcesses;
	int hitLimit = 0;	
	while(!hitLimit)
	{

	}

	// wait on all children before finishing
	int status;
	pid_t childpid;
	while((childpid = wait(&status)) > 0)
		--numProcesses;
	
	if (childPids != NULL)
	{
		free(childPids);
	}

	deallocateAllSharedMemory(argv[0]);
	return 0;
}

void displayHelp()
{

}

void deallocateAllSharedMemory(char* processName)
{
	deallocateSharedMemory(shmidSeconds, processName);
	deallocateSharedMemory(shmidNanoSeconds, processName);
	deallocateMessageQueue(msgIdCriticalSection, processName);
	deallocateMessageQueue(msgIdSend, processName);
}

void signalInterruption(int signo)
{
	if (signo == SIGALRM || signo == SIGINT)
	{
		deallocateAllSharedMemory(processName);
	}	
}
