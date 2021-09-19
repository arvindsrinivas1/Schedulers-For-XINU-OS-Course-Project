/* ready.c - ready */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <sched.h>
/*------------------------------------------------------------------------
 * ready  --  make a process eligible for CPU service
 *------------------------------------------------------------------------
 */
int ready(int pid, int resch)
{
	register struct	pentry	*pptr;

	if (isbadpid(pid))
		return(SYSERR);
	pptr = &proctab[pid];
	pptr->pstate = PRREADY;
	insert(pid,rdyhead,pptr->pprio);
	if(schedule_class != LINUXSCHED){
	//We are not allowing resch to be called for linux sched as we dont want it to interrupt our existing process
	//The process will still be put in the ready queue so it will still be considered to run if it already present during the begining of the epoch
	//This is just to stop new processes calling resch as they should not be run in the current epoch
		if (resch)
			resched();
	}
	return(OK);
}
