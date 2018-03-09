// user.c
// User process which will run inside the oss executeable
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

int main(int argc, char* argv[])
{
	// Can't seed with time(NULL) because it doesn't vary enough (1 second), so
	// use the pid as well so that children don't end up running with the same 
	// random number
	srand(getpid() * time(NULL));
	int nanoSecondsToRun = (rand() % 100000) + 1;
	int nanoSecondsRan = 0;
#ifdef DEBUG_OUTPUT
	printf("Spawned child process %d\n", getpid());
	printf("%d will run for %d nano seconds\n", getpid(), nanoSecondsToRun);
#endif
	int* seconds = getExistingSharedMemory(INT_SECONDS_KEY, argv[0]);
	int* nanoSeconds = getExistingSharedMemory(INT_NANO_SECONDS_KEY, argv[0]);
	
	int msgCriticalSectionId = getExistingMessageQueue(INT_CRITICAL_MESSAGE, argv[0]);
	if (msgCriticalSectionId == -1)
	{
		writeError("Failed to get message queue for critical section", argv[0]);
		return -1;
	}
	
	int msgSendId = getExistingMessageQueue(INT_COMMUNICATION_MESSAGE, argv[0]);
	if (msgSendId == -1)
	{
		writeError("Failed to get message queue for communication to oss", argv[0]);
		return -1;
	}

#ifdef DEBUG_OUTPUT
	printf("MSGID Critical: %d\nMSGID Send Queue: %d\n", msgCriticalSectionId, msgSendId);
	printf("Initial Seconds value: %d\n", *seconds);
	printf("Initial nano seconds value: %d\n", *nanoSeconds);
#endif
	
	// Objects to hold messages, though critical second sends no real info beyond
	// the presence of a message in the queue	
	struct msgbuf buf;
	struct msgbuf crit;
	
	// Continually try to enter critical section and "progress" through child process 
	while(1)
	{
		// Try to enter critical section and then block until it's available
		int bytesRead = 0;
		if ((bytesRead = msgrcv(msgCriticalSectionId, &crit, sizeof(crit), 1)) == -1)
		{
			writeError("Failed to receive critical section message", argv[0]);
		}
		
		// Generate an increment of nanoseconds of "work" to do
		int nanoSecondsToWork = rand() % 10000;
		
		if (nanoSecondsToWork + nanoSecondsRan >= nanoSecondsToRun)
		{
			// We're done, finish up and send log to oss
			nanoSecondsToWork = nanoSecondsToRun - nanoSecondsRan;
			*nanoSeconds += nanoSecondsToWork;
			if (*nanoSeconds >= NANO_PER_SECOND)
			{
				*seconds = *seconds + 1;
				*nanoSeconds -= NANO_PER_SECOND;			
			}
			
			snprintf(buf.mText, 200,  "Master: Child process %d terminated at system time %d.%d because it ran for 0.%d seconds \n", getpid(), *seconds, *nanoSeconds, nanoSecondsToRun);
			crit.mtype = 1;
			if (msgsnd(msgCriticalSectionId, &crit, sizeof(crit), 0) == -1)
			{
				writeError("Failed to give up critical section in child", argv[0]);
			}
			
			break;
		}
		
		nanoSecondsRan += nanoSecondsToWork;
		
#ifdef DEBUG_OUTPUT
		printf("Current child %d ran nanoSeconds %d out of %d total\n", getpid(),  nanoSecondsRan, nanoSecondsToRun);
#endif
		*nanoSeconds += nanoSecondsToWork;
		if (*nanoSeconds >= NANO_PER_SECOND)
		{
			*seconds = *seconds + 1;
			*nanoSeconds -= NANO_PER_SECOND;			
		}


		crit.mtype = 1;
		// Send message to give up control of the critical section
		if(msgsnd(msgCriticalSectionId, &crit, sizeof(crit), 0) == -1)
		{
			writeError("Failed to give up critical section", argv[0]);
		}
	}

#ifdef DEBUG_OUTPUT
	printf("User %d is about to send message back to parent\n", getpid());
#endif

	buf.mtype = 2;
	// Send message back to oss about running time
	if (msgsnd(msgSendId, &buf, sizeof(buf), 0) == -1)
	{
		writeError("Failed to send message to parent", argv[0]);
	}
	
	return 0;
}
