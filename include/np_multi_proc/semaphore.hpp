#pragma once
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>

static struct sembuf	op_lock[2] = {
	0, 0, 0,		/* wait for sem#0 to become 0 */
	0, 1, SEM_UNDO	/* then increment sem#0 by 1 */
};

static struct sembuf	op_unlock[1] = {
	0, -1, (IPC_NOWAIT | SEM_UNDO)	
				/* decrement sem#0 by 1 (sets it to 0) */
};

void lock(int semid){
    if (semop(semid, &op_lock[0], 2) < 0){
		perror("semop lock error");
    }
}
void unlock(int semid){
    if (semop(semid, &op_unlock[0], 1) < 0){
		perror("semop unlock error");
    }
}