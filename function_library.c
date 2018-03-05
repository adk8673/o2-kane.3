// Alex Kane
// 2/7/2018
// CS 4760 Project 1
// function_library.c
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include "function_library.h"

#define BILLION 1000000000L

enum state { idle = 0, want_in = 1, in_cs = 2};

// Check c-string to detemine if it contains
// only numbers
int checkNumber(const char* inputValue)
{
	int isNum = 1;
	const char* c = inputValue;
	
	// use is digit to determine if each character is a digit
	while (isNum && *c != '\0')
	{
		if (!isdigit(*c))
			isNum = 0;
		++c;
	}
	
	return isNum;
}

// Using perror, write information about errors to stderr
void writeError(const char* errorMessage, const char* processName)
{
	char message[1024];
	
	snprintf(message, sizeof(message), "%s: Error: %s", processName, errorMessage);
	
	perror(message);
}

// Create a child process by forking a child and executing the target process
// using command line arguements passed
pid_t createChildProcess(const char* targetProgram, const char* processName)
{
	pid_t forkedPid = fork();

	// see if fork is child or parent
	// also check for errors in forking
	if (forkedPid == 0)
	{
		// If we get here, we are inside of child process, begin parsing and our target program
		// and args into a command line
		char** args;
		int execResults;
		int numItems  = makeargv(targetProgram, " ", &args);
		execResults = execvp(args[0], args);
		if (execResults == -1)
		{
			printf("%d\n", errno);
			writeError("Failed to exec targetProcess", processName);
		}
	}
	else if (forkedPid < 0)
	{
		writeError("Failed to fork process", processName);
	}

	return forkedPid;	
}

// Function taken from Robbins textbook
int makeargv(const char *s, const char *delimiters, char ***argvp)
{
	int error;
	int i;
	int numtokens;
	const char *snew;
	char* t;

	if ((s == NULL) || (delimiters == NULL) || (argvp == NULL))
	{
		errno = EINVAL;
		return -1;
	}
	
	*argvp = NULL;
	snew = s + strspn(s, delimiters);
	if ((t = malloc(strlen(snew) + 1 )) == NULL)
		return -1;
	strcpy(t, snew);
	numtokens = 0;
	if (strtok(t, delimiters) != NULL)
		for (numtokens = 1; strtok(NULL, delimiters) != NULL; numtokens++) ;

	if ((*argvp = malloc((numtokens + 1) * sizeof(char *))) == NULL)
	{
		error = errno;
		free(t);
		errno = error;
		return -1;
	}
	
	if (numtokens == 0)
		free(t);
	else 
	{
		strcpy(t, snew);
		**argvp = strtok(t, delimiters);
		for (i = 1; i < numtokens; i++)
			*((*argvp) + i) = strtok(NULL, delimiters);
	}

	*((*argvp) +  numtokens) = NULL;
	return numtokens;
}

key_t getKey(int id)
{
        key_t key = -1;
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        key = ftok(cwd, id);
        return key;
}

int allocateSharedMemory(int id, const char* processName)
{
	const int memFlags = (0777 | IPC_CREAT);	
	int shmid = 0;
	key_t key = getKey(id);
	if ((shmid = shmget(key, sizeof(int), memFlags)) == -1)
	{
		writeError("Failed to allocated shared memory for key", processName);
	}

	return shmid;
}	

int allocateMessageQueue(int id, const char* processName)
{
	const int msgFlags = (0777 | IPC_CREAT);
	int msgid = 0;
	key_t key = getKey(id);
	if ((msgid = msgget(key, msgFlags)) == -1)
	{
		writeError("Failed to allocate message queue for key", processName);
	}

	return msgid;
}

int getExistingMessageQueue(int id, const char* processName)
{
	const int msgFlags = (0777);
	int msgid = 0;
	key_t key = getKey(id);
	if ((msgid = msgget(key, msgFlags)) == -1)
	{
		writeError("Failed to get existing message queue for key", processName);
	}
	
	return msgid;
}
void deallocateMessageQueue(int msgID, const char* processName)
{
	if(msgctl(msgID, IPC_RMID, NULL) == -1)
		writeError("Failed to deallocate message queue", processName);
}

void* getExistingSharedMemory(int id, const char* processName)
{
	key_t key = getKey(id);
	void* pResult = NULL;
	int shmid = -1;
	if ((shmid = shmget(key, 0, 0777)) == -1)
	{
		writeError("Failed to get SHMID of existing memory", processName);
	}
	else
	{
		pResult = shmat(shmid, 0, 0);
		if ((int)pResult == -1)
		{
			pResult = NULL;
			writeError("Failed to map existing shared memory to local address space", processName);
		}
	}
	
	return pResult;
}

void deallocateSharedMemory(int shmid, const char* processName)
{
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		writeError("Failed to deallocated shared memory for key", processName);
}

// function taken from Robbins textbook
int setPeriodic(double sec)
{
	time_t timerid;
	struct itimerspec value;

	if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1)
		return -1;

	value.it_interval.tv_sec = (long)sec;
	value.it_interval.tv_nsec = (sec- value.it_interval.tv_sec) * BILLION;
	if (value.it_interval.tv_nsec >= BILLION)
	{
		value.it_interval.tv_sec++;
		value.it_interval.tv_nsec -= BILLION;
	}	
	
	value.it_value = value.it_interval;
	return timer_settime(timerid, 0, &value, NULL);
}


