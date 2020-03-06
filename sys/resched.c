#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include<lab1.h>

unsigned long currSP;	/* REAL sp of current process */
extern int ctxsw(int, int, int, int);
int resched_class=0;
/*-----------------------------------------------------------------------
 *  * resched  --  reschedule processor to highest priority ready process
 *   *
 *    * Notes:	Upon entry, currpid gives current process id.
 *     *		Proctab[currpid].pstate gives correct NEXT state for
 *      *			current process if other than PRREADY.
 *       *------------------------------------------------------------------------
 *        */   
int resched()
{
	register struct	pentry	*optr;	/* pointer to old process entry */
	register struct	pentry	*nptr;	/* pointer to new process entry */

	/* no switch needed if current process priority higher than next*/

	//RANDOMSCHED SCHEDULER
	if(resched_class==RANDOMSCHED)
	{
		optr=&proctab[currpid];
		/*
		if(currpid==NULLPROC) /* to check for null process */
		//{
		//	return SYSERR;
		//}

		
		if(optr->pstate == PRCURR)
		{
			optr->pstate = PRREADY;
	        insert(currpid,rdyhead,optr->pprio);
		}

		currpid = randomgenerator();
		//kprintf("%d",currpid);
		dequeue(currpid);			/*remove the process from ready queue*/
		//nptr = &proctab[currpid];
		(nptr=&proctab[currpid])->pstate = PRCURR;		/* mark it currently running	*/
	#ifdef	RTCLOCK
		preempt = QUANTUM;		/* reset preemption counter	*/
	#endif	
		
		ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask); //context switch old process and new process
	
		/* The OLD process returns here when resumed. */
		return OK;
	}

	//LINUXSCHED SCHEDULER

	else if(resched_class==LINUXSCHED)
	{

		int i=0;
		optr = &proctab[currpid];
		optr->goodness -= optr->counter;
		optr->goodness+=preempt;
		optr->counter = preempt;
		
		// NULL process
		if ( optr->counter <=0 || currpid == NULLPROC)
		{
			optr->goodness = 0;
			optr->counter = 0;
		}
		int maxgoodness=highestgoodness();
		int current=highestpriority_proc();
		if (optr->counter == 0||optr->pstate!=PRCURR) //when new epoch is created
		{
			if (maxgoodness == 0)
			{
				initialize_new_epoch();

			
			preempt = optr->counter;
			
			if (currpid == NULLPROC)
			{
				return OK;
			}
			else if (currpid != NULLPROC)
			{
				current = NULLPROC;
				#ifdef	RTCLOCK
					preempt = QUANTUM;
				#endif		 		
			}
			}
		}
		else if (optr->goodness > 0)
		{
			if (optr->goodness >= maxgoodness)
			{
				if (optr->pstate != PRCURR)
				{
					optr->goodness=optr->counter;
					i=optr->goodness+(int)optr->goodness/2;
				}
				else
				{
					preempt = optr->counter;
					return(OK);
				}
			}
		}
		
		if (optr->pstate == PRCURR) 
		{
				optr->pstate = PRREADY;
				insert(currpid, rdyhead, optr->pprio);
		}

		nptr = &proctab[current];
		nptr->pstate = PRCURR;
		dequeue(current);
		currpid = current;
		preempt = nptr->counter;
		
		ctxsw((int) &optr->pesp, (int) optr->pirmask, (int) &nptr->pesp, (int) nptr->pirmask);
		
	return OK;
}

//Default Scheduler code when both schedulers won't run

	if ((optr->pstate == PRCURR) &&
	   (lastkey(rdytail)<optr->pprio)) 
	{
		return(OK);
	}
	
	/* force context switch */

	if((optr=&proctab[currpid])->pstate == PRCURR) 
	{
		optr->pstate = PRREADY;
		insert(currpid,rdyhead,optr->pprio);
	}

	/* remove highest priority process at end of ready list */

	nptr = &proctab[ (currpid = getlast(rdytail)) ];
	nptr->pstate = PRCURR;		/* mark it currently running	*/
#ifdef	RTCLOCK
	preempt = QUANTUM;		/* reset preemption counter	*/
#endif
	
	ctxsw((int)&optr->pesp, (int)optr->pirmask, (int)&nptr->pesp, (int)nptr->pirmask);
	
	/* The OLD process returns here when resumed. */
	return OK;
}

void initialize_new_epoch()
{
	int n = 0;
			struct pentry *proc;
			for(n=0;n<NPROC;n+=1)
			{
				proc = &proctab[n];
				if (proc->pstate != PRFREE)
				{
					proc->counter = proc->pprio + (int)(proc->counter)/2;
				}
				// Updating the goodness value
				proc->goodness =  proc->pprio + proc->counter;
			}
			
}
int highestgoodness()
{
	int i=0,maxgoodness=0,current=0;
	for(i=q[rdytail].qprev;i!=rdyhead;i=q[i].qprev) //Process to find the maximum goodness
		{
			if(proctab[i].goodness > maxgoodness)
			{
				maxgoodness = proctab[i].goodness;
				current = i;
			}
		}
		return maxgoodness;
}
int highestpriority_proc()
{
	int i=0,maxgoodness=0,current=0;
	for(i=q[rdytail].qprev;i!=rdyhead;i=q[i].qprev) //Process to find the maximum goodness
		{
			if(proctab[i].goodness > maxgoodness)
			{
				maxgoodness = proctab[i].goodness;
				current = i;
			}
		}
		return current;
}
int randomgenerator()
{
	int sum=0,curpid=0,i=0;
	//for(i=q[rdytail].qprev;i!=rdyhead;i=q[i].qprev)
	for(i=q[rdyhead].qnext;i!=rdytail;i=q[i].qnext)
	{
		sum += q[i].qkey;		//calculate the sum of all process priorities
	}
	if(sum > 0) //To stop generating random number if there is null process in the ready queue
	{
		int number = rand()%sum;			//generate a random number in the range 0 to sum-1
		curpid=q[rdytail].qprev;
		if(curpid==rdyhead)
		{
			curpid=0;
		}
		else
		{
		for(i=q[rdytail].qprev;i!=rdyhead;i=q[i].qprev)
		{
			curpid = i;
            if(number > q[i].qkey)
            {	
				number -= q[i].qkey;
			}
			else 
			{
				break;
			}
        }
        if(curpid==rdyhead)
        {
        	curpid=q[curpid].qnext;
        }

    }
	/*
	if(sum==0)
	{
		kprintf("Null process");
	}
	*/
}
	return curpid;
}
void setschedclass(int sched_class){
         resched_class = sched_class;
 }

 int getschedclass(){
        return resched_class;
 }

