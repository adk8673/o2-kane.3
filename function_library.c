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

/*
void process(const int i, const int n, int* turn, int* flag)
{
	int j;
	do 
	{
		do
		{
			flag[i] = (int)want_in;
			j = *turn;
			while (j != i)
				j = (flag[j] != idle) ? *turn : (j + 1) % n;

			// declare intention to enter critical section
			flag[i] = in_cs;

			// check that no one else is in the critical section
			for (j = 0; j < n; j++)
				if ((j != i) && (flag[j] == in_cs))
					break;
		} while ((j < n) || (*turn != i && flag[*turn] != idle));
		
		// Assign turn to self and enter critical section
		*turn = i;
		
		sleep(1);

		//Exit section
		j = (*turn + 1) % n;
		while (flag[j] == idle)
		{
			j = (j + 1) % n;
		}
		
		//Assign turn to next waiting process; change own flag to idle 
		*turn = j; flag[i] = idle;
		sleep(2);
	}while(1); 
}*/
