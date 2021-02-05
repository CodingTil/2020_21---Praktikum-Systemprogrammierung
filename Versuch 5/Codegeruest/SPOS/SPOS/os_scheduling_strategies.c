#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

SchedulingInformation schedulingInfo;

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    if(strategy == OS_SS_ROUND_ROBIN) {
		schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
	}else if(strategy == OS_SS_INACTIVE_AGING) {
		for(uint8_t i = 0; i < MAX_NUMBER_OF_PROCESSES; i++) {
			schedulingInfo.age[i] = 0;
		}
	} else if (strategy == OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE) {
		os_initSchedulingInformation();
	}
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    schedulingInfo.age[id] = 0;
	schedulingInfo.remainingts[id] = MLFQ_getDefaultTimeslice(os_getProcessSlot(id)->priority);
}

/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current) {
    for(uint8_t i = current + 1; i <= MAX_NUMBER_OF_PROCESSES + current; i++) {
		if(processes[i % MAX_NUMBER_OF_PROCESSES].state == OS_PS_READY && i % MAX_NUMBER_OF_PROCESSES != 0) return i % MAX_NUMBER_OF_PROCESSES;
	}
	return 0;
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current) {
	uint8_t number_active_procs = os_getNumberOfActiveProcs();
    if(number_active_procs == 1) return 0; //Only idle process
    uint8_t active_found = 0;
    uint8_t result = rand() % (number_active_procs - 1);
	for(uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if(processes[i].state == OS_PS_READY) {
			if(active_found++ == result) return i;
		}
	}
	return 0; // For the compiler
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current) {
    if(processes[current].state != OS_PS_READY || schedulingInfo.timeSlice == 1) {
		ProcessID id = os_Scheduler_Even(processes, current);
		schedulingInfo.timeSlice = processes[id].priority;
		return id;
	}
	schedulingInfo.timeSlice -= 1;
	return current;
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current) {
	// Update aged
	uint8_t i;
	for (i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if (processes[i].state == OS_PS_READY && i != current) {
			schedulingInfo.age[i] += processes[i].priority;
		}
	}
	
	// Select highest
	uint8_t max_age = 0;
	for (i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if (processes[i].state == OS_PS_READY && schedulingInfo.age[i] > schedulingInfo.age[max_age]) {
			max_age = i;
		}
		if (processes[i].state == OS_PS_READY && schedulingInfo.age[i] == schedulingInfo.age[max_age] && processes[i].priority > processes[max_age].priority) {
			// if processes[i].priority == processes[max_age].priority then do nothing because we traverse array in correct order
			max_age = i;
		}
	}
	
	schedulingInfo.age[max_age] = processes[max_age].priority;

	return max_age;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current) {
    if(processes[current].state == OS_PS_UNUSED) return os_Scheduler_Even(processes, current);
	return current;
}




//Start of Implementation of functions for Multilevel-Feedback-Queue
void pqueue_init(ProcessQueue *queue) {
	queue->tail =  0;
	queue->head = 0;
	queue->size = MAX_NUMBER_OF_PROCESSES;
}

//Resets buffer
void pqueue_reset(ProcessQueue *queue) {
	queue->head = 0;
	queue->tail = 0;
}

//Checks whether there is next ProcessID
uint8_t pqueue_hasNext(ProcessQueue *queue) {
	uint8_t check = queue->tail;
	if (check != queue->head) {
		return queue->data[queue->tail];
	}
	return 0;
}

// Functions for reading and writing
ProcessID pqueue_getFirst(ProcessQueue *queue) {
	return pqueue_hasNext(queue);
}

void pqueue_dropFirst(ProcessQueue *queue) {
	if (pqueue_hasNext(queue)) {
		queue->data[queue->tail] = 0;
		queue->tail++;
		queue->tail = queue->tail % MAX_NUMBER_OF_PROCESSES;
	}
}

void pqueue_append(ProcessQueue *queue, ProcessID pid) {
	if (queue->head == MAX_NUMBER_OF_PROCESSES) {
		queue->data[0] = pid;
		queue->head = 0;
	} else {
		queue->data[queue->head + 1] = pid;
		queue->head++;
	}

}


ProcessQueue* MLFQ_getQueue(uint8_t queueID) {
	if (queueID >= 0 && queueID < 4) {
		return &(schedulingInfo.queues[queueID]);
	}
	return 0;
}

void os_initSchedulingInformation() {
	pqueue_init(&(schedulingInfo.queues[0]));
	pqueue_init(&(schedulingInfo.queues[1]));
	pqueue_init(&(schedulingInfo.queues[2]));
	pqueue_init(&(schedulingInfo.queues[3]));
	schedulingInfo.timeslices[0] = 1;
	schedulingInfo.timeslices[1] = 2;
	schedulingInfo.timeslices[2] = 4;
	schedulingInfo.timeslices[3] = 8;
	
}

ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current) {
	order_blocked_processes();
	ProcessID pid;
	for (int i = 0; i < 4; i++) {
		pid = pqueue_hasNext(&(schedulingInfo.queues[i])); 
		if (pid && os_getProcessSlot(pid)->state == OS_PS_READY) {
			ProcessID chosen = pqueue_getFirst(&(schedulingInfo.queues[i]));
			/*
			each process gets default timeslices after the 
			initialization due to function os_resetProcessSchedulingInformation --> the
			value is stored in array remainingtimeslice at the index of the ProcessID of 
			the process
			*/
			schedulingInfo.remainingts[chosen]--;
			if (schedulingInfo.remainingts[chosen] == 0) {
				pqueue_dropFirst(&(schedulingInfo.queues[i]));
				if (i < 3) {
					pqueue_append(&(schedulingInfo.queues[i+1]), chosen);
					schedulingInfo.remainingts[chosen] = MLFQ_getDefaultTimeslice(i+1);
				} else {
					//process remains in class 4 if it still needs processing time
					if(processes[chosen].state == OS_PS_READY) {
						pqueue_append(&(schedulingInfo.queues[3]),chosen);
						schedulingInfo.remainingts[chosen] = MLFQ_getDefaultTimeslice(3);
					}
				}
			}
			/*
			Blocked processes were not considered in this request --> 
			set them ready to guarantee that blocked process are only ignored once -->
			available next scheduling
			*/
			for (int j = 0; j < sizeof(&processes); j++) {
				if (processes[j].state == OS_PS_BLOCKED) {
					os_getProcessSlot(j)->state = OS_PS_READY;
				}
			}
			return chosen;
		}
	}
	//idle process
	return 0;
}


/*
helper function order_blocked_processes appends blocked processes to the end of the 
Process Queue in each Queue
*/
void order_blocked_processes() {
	for (int i = 0; i < 4; i++) {
		ProcessID check = pqueue_hasNext(&(schedulingInfo.queues[i]));
		if ((uint16_t) os_getProcessSlot(check) == OS_PS_BLOCKED) {
			pqueue_dropFirst(&(schedulingInfo.queues[i]));
			pqueue_append(&(schedulingInfo.queues[i]),check);
		}
		while(pqueue_hasNext(&(schedulingInfo.queues[i])) == OS_PS_BLOCKED && check != pqueue_hasNext(&(schedulingInfo.queues[i]))) {
			pqueue_dropFirst(&(schedulingInfo.queues[i]));
			pqueue_append(&(schedulingInfo.queues[i]),check);
		}
	}
}

void MLFQ_removePID(ProcessID pid) {
	for (int i = 0; i < 4; i++) {
		for (uint8_t q = 0; q < MAX_NUMBER_OF_PROCESSES; q++) {
			if (schedulingInfo.queues[i].data[q] == pid) {
				schedulingInfo.queues[i].data[q] = 0;	
			}
		}
	}
}

uint8_t MLFQ_getDefaultTimeslice(uint8_t queueID) {
	return schedulingInfo.timeslices[queueID];
}

uint8_t MLFQ_MapToQueue(Priority prio) {
	return (prio >> 6);
}