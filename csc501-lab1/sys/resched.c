/* resched.c  -  resched */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <math.h>
#include <sched.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);

int goodness_value[NPROC];
int counter[NPROC];
int quantum[NPROC];
int eligible[NPROC];
int priority_in_curr_epoch[NPROC];


int calc_quantum(){
        int i = 0;
        while(i < NPROC){
                if (proctab[i].pstate != PRFREE){
                        if(counter[i] == 0 ){
                                quantum[i] = proctab[i].pprio;
                                counter[i] = quantum[i];
                        }
                        else{
                                quantum[i] = ((int)counter[i]/2) + proctab[i].pprio;//calc during new opoch only so I can use pprio 
                                counter[i] = quantum[i];
                        }
                }
		i++;
        }
}

int epoch_check(){
        int nxt = q[rdyhead].qnext;
        if(q[rdyhead].qnext == rdytail && q[rdytail].qprev == rdyhead){
               // if(proctab[currpid].pstate == PRCURR && preempt != 0){
                 //       kprintf("This can never happen\n");
               // }
                return(0);
        }
        while(nxt < NPROC){
                if(counter[nxt] != 0 && proctab[nxt].pstate == PRREADY){
                        return(1);
                }

                nxt = q[nxt].qnext;
        }

        return(0);
}


int refresh_priorities(){
        int i;
        for(i=0; i<NPROC; i++){
                priority_in_curr_epoch[i] = proctab[i].pprio;
        }
}

int calc_goodness_value(){
        int i = 1;
        for(i=1; i<NPROC; i++){
		if(proctab[i].pstate == PRFREE){
			goodness_value[i] = 0;
		}
                if(counter[i] == 0){
                        goodness_value[i] = 0;
                }
                else if(proctab[i].pstate == PRREADY){
                        goodness_value[i] = counter[i] + priority_in_curr_epoch[i];
                }
        }
}

int get_next_process(){
        int i = 1,max_goodness_position = 1;
        for(i=1; i<NPROC; i++){
                if(counter[i] != 0 && proctab[i].pstate==PRREADY){
                        if(goodness_value[i] > goodness_value[max_goodness_position]){
                                max_goodness_position = i;
                        }
                }
        }
	return(max_goodness_position);
}

/*-----------------------------------------------------------------------
 * resched  --  reschedule processor to highest priority ready process
 *
 * Notes:	Upon entry, currpid gives current process id.
 *		Proctab[currpid].pstate gives correct NEXT state for
 *			current process if other than PRREADY.
 *------------------------------------------------------------------------
 */
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */
	int next;
	double randval;
	int epoch_alive =0;
	int next_process = 0;
	
	optr = &proctab[currpid];
	// By default I'm running exp scheduler. This will be used initially. 322 is the default value i've given.
	// I'm handling nullproc by not putting it in rdyqueue. This is because we wont use the null proc until the
	// rdy queue is empty anyway.
	if(schedule_class == EXPDISTSCHED || schedule_class == 322){
		randval = (int) expdev(0.1);
	if(q[rdyhead].qnext == rdytail && q[rdytail].qprev == rdyhead){
		//empty rdyqueue
		if(optr->pstate == PRCURR){
			#ifdef RTCLOCK
				preempt = QUANTUM;
			#endif
			
			return OK;
		}
		else{
			//context switch with null proc
			//not dequeueing because nullproc is not kept in the rdyqueue
			currpid = NULLPROC;
			nptr = &proctab[NULLPROC];
			nptr->pstate = PRCURR;
			#ifdef RTCLOCK
				preempt = QUANTUM;
			#endif
			ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
			return OK;
		}

	}
	
	//else the rdyqueue is not empty
	
	next = q[rdyhead].qnext;	
	if(randval < q[next].qkey){
		if(optr->pstate == PRCURR && (optr->pprio > randval) && (optr->pprio < q[next].qkey) ){
			#ifdef RTCLOCK
				preempt = QUANTUM;
			#endif
			return OK;
		}
		else{
			if(optr->pstate == PRCURR){
				optr->pstate = PRREADY;
				if(currpid != NULLPROC){
					insert(currpid, rdyhead, optr->pprio);
				}		
			}
			if(next < NPROC){
				dequeue(next);
			}
			currpid = next;
			nptr = &proctab[currpid];
			nptr->pstate = PRCURR;
			#ifdef RTCLOCK
				preempt = QUANTUM;
			#endif
			ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
			return OK;
		}
	}
	else{
		while(q[next].qkey <= randval && next < NPROC){
			next = q[next].qnext;
		}
		//max value of next here can be rdytail	
		if(next == rdytail){
			if(optr->pstate == PRCURR && (optr->pprio > randval ||  (optr->pprio < randval && optr->pprio > q[q[next].qprev].qkey) )){
				#ifdef RTCLOCK
					preempt = QUANTUM;
				#endif
				return OK;
			}
			else{
				if(optr->pstate == PRCURR){
					optr->pstate == PRREADY;
					if(currpid != NULLPROC){
						insert(currpid, rdyhead, optr->pprio);
					}
				}
				currpid = getlast(rdytail); //already does dequeue
				nptr = &proctab[currpid];
				nptr->pstate = PRCURR;
				#ifdef RTCLOCK
					preempt = QUANTUM;
				#endif
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);	
				return OK;
			}
		}

		else{
			//Hence if currpid has prio same as next's prio then CS happens. Round robin is implemented.
			if(optr->pstate == PRCURR && (optr->pprio > randval && optr->pprio < q[next].qkey)){
				#ifdef RTCLOCK
					preempt = QUANTUM;
				#endif
				return OK;
			}
			else{
				if(optr->pstate == PRCURR){
					optr->pstate = PRREADY;
					if(currpid != NULLPROC){
						insert(currpid, rdyhead, optr->pprio);
					}
				}
				if(next < NPROC){
					dequeue(next);
				}
				currpid = next;
				nptr = &proctab[currpid];
				nptr->pstate = PRCURR;
				#ifdef RTCLOCK
					preempt = QUANTUM;
				#endif
				ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
				return OK;		
			}	
		}
	}


}//exp_sched_class

	else if(schedule_class == LINUXSCHED){
		// DO linux sched here
                optr = &proctab[currpid];
               	if(optr->pstate != PRFREE){
			 counter[currpid] = preempt;
		}
		else{
			//Finished process will have some preempt left so we make counter as 0 instead of preempt
			counter[currpid] = 0;
		}
		 epoch_alive = epoch_check();
                //if epoch_alive == 0, means epoch has ended, so we should do the needfull
                //calculate new epoch max length? goodnessval, counter values, refresh priorities
                if(epoch_alive == 0){
                        refresh_priorities();
                        calc_quantum();
                        epoch_alive = 1;
                }

		calc_goodness_value();
                if(q[rdyhead].qnext == rdytail && q[rdytail].qprev == rdyhead){
                        if( currpid == NULLPROC ){
                                counter[NULLPROC] = QUANTUM;
                                #ifdef RTCLOCK
                                        preempt = counter[0];
                                #endif
                                return OK;
                        }
                        else if(currpid != NULLPROC){
				if(preempt != 0){
					currpid = NULLPROC;
					counter[NULLPROC] = QUANTUM;
					nptr = &proctab[NULLPROC];
					nptr->pstate = PRCURR;
					#ifdef RTCLOCK
						preempt = counter[NULLPROC];
					#endif
					ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
					return OK;
				}
				else{

					if(optr->pstate == PRCURR){
						#ifdef RTCLOCK
							preempt = counter[currpid];
						#endif
						return OK;
					}
				}
                                currpid = NULLPROC;
                                counter[NULLPROC] = QUANTUM;
                                #ifdef RTCLOCK
                                        preempt = counter[0];
                                #endif
                                return OK;
                        }
                }

                if(preempt != 0){
			// Can either mean the process has finished or wait/sleep/yield was called on the process
			// Either way we wont insert the process back into the queue
                        next_process = get_next_process();
                        currpid = next_process;
                        nptr = &proctab[currpid];
                        if(next_process != NULLPROC){
                                dequeue(next_process);
                        }
                        nptr->pstate = PRCURR;
                        #ifdef RTCLOCK
                                preempt = counter[next_process];
                        #endif
                        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                        return OK;
                }
		else{
                        optr->pstate = PRREADY;
                        if(currpid != NULLPROC && proctab[currpid].pstate != PRFREE){
			//PRFREE is needless but i've put here just in case of the edge case condition where preempt = 0 for finished process
                                insert(currpid, rdyhead, optr->pprio);
                        }
                        next_process = get_next_process();
                        currpid = next_process;
                        nptr = &proctab[currpid];
                        if(next_process != NULLPROC){
                                dequeue(next_process);
                        }
                        nptr->pstate = PRCURR;
                        #ifdef RTCLOCK
                                preempt = counter[next_process];
                        #endif
                        ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
                        return OK;

		}
	} 

	else {
	// DEAD CODE - THIS NEVER RUNS
	/* no switch needed if current process priority higher than next*/
	if ( ( (optr= &proctab[currpid])->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) {
		return(OK);
	}

	/* force context switch */

	if (optr->pstate == PRCURR) {
		optr->pstate = PRREADY;
		if(currpid != NULLPROC){
			insert(currpid,rdyhead,optr->pprio);
		}
	}
	/* remove highest priority process at end of ready list */
	if(q[rdyhead].qnext == rdytail && q[rdytail].qprev == rdyhead){
		nptr = &proctab[NULLPROC];
	}
	else{
	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	}
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);

	/* The OLD process returns here when resumed. */
	return OK;
	}
}
