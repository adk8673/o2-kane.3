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
#ifdef DEBUG_OUTPUT
	printf("Spawned child process %d\n", getpid());
#endif
	return 0;
}
