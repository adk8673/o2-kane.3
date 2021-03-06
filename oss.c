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

int totalProcessesCreated = 0;
char* processName;
FILE* masterLog = NULL;
pid_t* childPids = NULL;
  
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

	printf("MSGID crit: %d\nMSGID message: %d\n", msgIdCriticalSection, msgIdSend);
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
	
	setPeriodic(timeToTerminate);

	// if passed in a help flag, we don't want to do anything else - just exit
	if (argFlagHelp)
	{
		return 0;
	}
	
	masterLog = fopen(fileName, "w");
	struct msgbuf crit;
	struct msgbuf buf;

	// spawn off child processes and allocate variables for child tracking
	childPids = malloc(sizeof(pid_t) * MAX_NUM_PROCESSES );
	int i, totalProcessesCreated = 0;
	for (i = 0; i < numChildren; ++i)
	{
		pid_t childpid;
		childpid = createChildProcess("./user", argv[0]);
		childPids[i] = childpid;
		++totalProcessesCreated;

		fprintf(masterLog, "Master: Creating child pid %d at my time %d.%d\n", childpid, *seconds, *nanoSeconds);
	}

	// set initial access to critical section
	crit.mtype = 1;
	if(msgsnd(msgIdCriticalSection, &crit, sizeof(crit), 0) == -1)
	{
		writeError("Failed to initialize critical section\n", processName);
		return -1;
	}
	
	int hitLimit = 0;	
	while(!hitLimit && totalProcessesCreated < MAX_NUM_PROCESSES )
	{
#ifdef DEBUG_OUTPUT
		printf("oss waiting on message, total number of processes that have been created so far is: %d\n", totalProcessesCreated);
#endif
		int bytesReceived = 0;
		// wait for a child process to send a message back to this process to indicate
		if ((bytesReceived = msgrcv(msgIdSend, &buf, sizeof(buf), 2, 0))== -1)
		{
			writeError("Error in oss on receiving message\n", processName);
		}

#ifdef DEBUG_OUTPUT
		printf("oss received message: %s\n", buf.mText);
#endif

		// Child proccess has terminated, need to write out to a file and increment our counter
		// Try to enter critical section
		if (msgrcv(msgIdCriticalSection, &crit, sizeof(crit), 1, 0) == -1)
		{
			writeError("Failure to enter critical section\n", processName);
		}
	
		fprintf(masterLog, buf.mText);
		
		*nanoSeconds += 100;
		if (*nanoSeconds > NANO_PER_SECOND)
		{
			*nanoSeconds -= NANO_PER_SECOND;
			*seconds += 1;
		}
		
		++totalProcessesCreated;
		if (*seconds >= TOTAL_SECONDS_TIME)
		{
			hitLimit = 1;
		}
		else
		{
			pid_t childPid = createChildProcess("./user", argv[0]);
			fprintf(masterLog, "Master: Creating child pid %d at my time %d.%d\n", childPid, *seconds, *nanoSeconds);
			printf("Child was created %d\n", childPid);
			childPids[totalProcessesCreated] = childPid;
		}
		

		// Give up conrtol of critical section
		crit.mtype = 1;
		if (msgsnd(msgIdCriticalSection, &crit, sizeof(crit), 0) == -1)
		{
			writeError("Failed to sent critical section message", processName);
		}
	}

	// wait on all children before finishing
	int status;
	pid_t childpid;
	while((childpid = wait(&status)) > 0);
	printf("waiting all children\n");
	if (childPids != NULL)
	{
		free(childPids);
	}
	
	deallocateAllSharedMemory(argv[0]);
	fclose(masterLog);
	
	return 0;
}

void displayHelp()
{
	printf("oss: Operating system simulator\nOptions:\n-s x\tNumber of concurrent child processes to spawn\n-l filename\tFilename to output log to\n-t z\tNumber of seconds to run before forcing exit\n-h\tDisplay help options\n");
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
		
		if (masterLog != NULL)
		{
			fclose(masterLog);
		}
		
		int i;
		for (i = 0; i < totalProcessesCreated; ++i)
		{
			kill (childPids[i], SIGKILL);
		}
		
		if (childPids != NULL)
		{
			free(childPids);
		}
		
	
		exit(0);
	}	
}
