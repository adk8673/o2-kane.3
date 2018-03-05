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
	int nanoSecondsToRun = (rand() % 1000000) + 1;
	int nanoSecondsRan = 0;
#ifdef DEBUG_OUTPUT
	printf("Spawned child process %d\n", getpid());
	printf("%d will run for %d nano seconds\n", getpid(), nanoSecondsToRun);
#endif
	int* seconds = getExistingSharedMemory(INT_SECONDS_KEY, argv[0]);
	int* nanoSeconds = getExistingSharedMemory(INT_NANO_SECONDS_KEY, argv[0]);
	int msgCriticalSectionId = getExistingMessageQueue(INT_CRITICAL_MESSAGE, argv[0]);
	int msgSendId = getExistingMessageQueue(INT_COMMUNICATION_MESSAGE, argv[0]);

#ifdef DEBUG_OUTPUT
	printf("Initial Seconds value: %d\n", *seconds);
	printf("Initial nano seconds value: %d\n", *nanoSeconds);
#endif

	// Objects to hold messages, though critical second sends no real info beyond
	// the presence of a message in the queue	
	struct msgbuf buf;
	struct msgbuf crit;

	// Make sure we are the only one trying to read the seconds/nanoseconds
	msgrcv(msgCriticalSectionId, &crit, sizeof(crit), 1, 0);

	// Create initilization message and send it to parent
	snprintf(buf.mText, "Creating new child pid %d at my time %d.%d\n", getpid(), *seconds, *nanoSeconds);
	buf.mtype = 2;
	msgsnd(msgSendId, &buf, sizeof(buf), 0);
	
	// Signal that we are leaving critical section
	crit.mtype = 1;	
	msgsnd(msgCriticalSectionId, &crit, sizeof(crit), 0);

	// Continually try to enter critical section and "progress" through child process 
	while(nanoSecondsRan < nanoSeocndsToRun)
	{
		// Try to enter critical section and then block until it's available
		msgrcv(msgCriticalSectionId, &crit, sizeof(crit), 0);

		// Generate an increment of nanoseconds of "work" to do
		int nanoSecondsToWork = rand() % 10000;
		
		if (nanoSecondsToWork + nanSecondsRan > nanoSecondsToRun)
		{
			// We're done, finish up and send log to oss
			nanoSecondsToWork = nanoSecondsToRun - nanoSecondsRan;
		}
		
		nanoSecondsRan += nanoSecondsToWork;
		*nanoSeconds += nanoSecondsToWork;
		if (*nanoSeconds >= NANO_PER_SECOND)
		{
			*seconds = *seconds + 1;
			*nanoSeconds -= NANO_PER_SECOND;			
		}

		if (nanoSecondsRan >= nanoSecondsToRun)
		{
			snprintf(buf.mText, "Child process %d terminated at system time %d.%d because it ran for 0.%d seconds \n", getpid(), *seconds, *nanoSeconds, nanoSecondsToRun);
		}
		else
		{
			snprintf(buf.mText, "Child process %d ran for %d nanoseconds\n", nanSecondsToWork;
		}

		// Send message back to oss about running time
		msgsnd(msgSendId, &buf, sizeof(buf), 0);

		crit.mtype = 1;
		// Send message to give up control of the critical section
		msgsnd(msgCriticalSeciontId, &crit, sizeof(crit), 0);
	}

	return 0;
}
