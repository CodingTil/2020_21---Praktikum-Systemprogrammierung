#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

SchedulingInformation schedulingInfo;


//*****DIE GANZEN FUNKTOINEN HATTEN IMMER EIN CONST: LAUT DOCS IST DAS AUCH SO: ABER ICH KANN DEN STATE NICHT ÄNDERN WENN DIE OBJECTE CONST SIND!!!!********

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
	}
}

// hier soll noch was geändert werden aber keine Ahnung was...

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    schedulingInfo.age[id] = 0;
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
ProcessID os_Scheduler_Even(Process processes[], ProcessID current) {
    for(uint8_t i = current + 1; i <= MAX_NUMBER_OF_PROCESSES + current; i++) {
		if(processes[i % MAX_NUMBER_OF_PROCESSES].state == OS_PS_READY && i % MAX_NUMBER_OF_PROCESSES != 0) return i % MAX_NUMBER_OF_PROCESSES;
		if(processes[i % MAX_NUMBER_OF_PROCESSES].state == OS_PS_BLOCKED) {
			processes[i % MAX_NUMBER_OF_PROCESSES].state = OS_PS_READY;
		}
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
ProcessID os_Scheduler_Random(Process processes[], ProcessID current) {
	uint8_t number_active_procs = os_getNumberOfActiveProcs();
    if(number_active_procs == 1) return 0; //Only idle process
    uint8_t active_found = 0;
    uint8_t result = rand() % (number_active_procs - 1);
	for(uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if(processes[i].state == OS_PS_READY) {
			if(active_found++ == result) return i;
		}
		if(processes[i].state == OS_PS_BLOCKED) processes[i].state = OS_PS_READY;

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
ProcessID os_Scheduler_RoundRobin(Process processes[], ProcessID current) {
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
ProcessID os_Scheduler_InactiveAging(Process processes[], ProcessID current) {
	// Update aged
	uint8_t i;
	for (i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if (processes[i].state == OS_PS_READY && i != current) {
			schedulingInfo.age[i] += processes[i].priority;
		}
		if(processes[i].state == OS_PS_BLOCKED) {
			processes[i].state = OS_PS_READY;
			schedulingInfo.age[i] = processes[i].priority;
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

ProcessID os_Scheduler_MLFQ(Process processes[], ProcessID current) {
	for (uint8_t i = 0; i < PRIORITY_CLASSES; i++) {
		if (pqueue_hasNext(&schedulingInfo.queues[i])) {
			ProcessID newPID = pqueue_getFirst(&schedulingInfo.queues[i]);
			if (newPID == current) {
				// no new process with lower priority or os_yield
				if (schedulingInfo.currentSlicesCounter == 0) {
					pqueue_dropFirst(&schedulingInfo.queues[i]);
					if (i + 1 == PRIORITY_CLASSES || processes[current].state == OS_PS_BLOCKED) {
						pqueue_append(&schedulingInfo.queues[i], current);
						processes[current].state = OS_PS_READY;
					} else {
						pqueue_append(&schedulingInfo.queues[i+1], current);
					}
					if (pqueue_hasNext(&schedulingInfo.queues[i])) {
						schedulingInfo.currentSlicesCounter = schedulingInfo.zeitscheiben[i] - 1;
						return pqueue_getFirst(&schedulingInfo.queues[i]);
					} else {
						continue;
					}
				} else {
					schedulingInfo.currentSlicesCounter--;
					return newPID;
				}
			} else {
				// new process found.
				schedulingInfo.currentSlicesCounter = schedulingInfo.zeitscheiben[i] - 1;
				return newPID;
			}
		}
	}
	return 0;
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
ProcessID os_Scheduler_RunToCompletion(Process processes[], ProcessID current) {
    if(processes[current].state == OS_PS_UNUSED) return os_Scheduler_Even(processes, current);
	return current;
}

void pqueue_init(ProcessQueue *queue) {
	//queue->data = {0};
	queue->size = MAX_NUMBER_OF_PROCESSES;
	queue->head = 0;
	queue->tail = 0;
}

void pqueue_reset(ProcessQueue *queue) {
	queue->head = 0;
	queue->tail = 0;
}

uint8_t pqueue_hasNext(ProcessQueue *queue) {
	return queue->head != queue->tail;
}

ProcessID pqueue_getFirst(ProcessQueue *queue) {
	return queue->data[queue->tail];
}

void pqueue_dropFirst(ProcessQueue *queue) {
	queue->tail++;
	if (queue->tail >= queue->size) {
		queue->tail = 0;
	}
}

void pqueue_append(ProcessQueue *queue, ProcessID pid) {
	queue->data[queue->head] = pid;
	if (queue->head >= queue->size) {
		queue->head = 0;
	}
}

ProcessQueue* MLFQ_getQueue(uint8_t queueID) {
	return &schedulingInfo.queues[queueID];
}

void os_initSchedulingInformation() {
	schedulingInfo.zeitscheiben[0] = 1;
	schedulingInfo.zeitscheiben[1] = 2;
	schedulingInfo.zeitscheiben[2] = 4;
	schedulingInfo.zeitscheiben[3] = 8;
	pqueue_init(&schedulingInfo.queues[0]);
	pqueue_init(&schedulingInfo.queues[1]);
	pqueue_init(&schedulingInfo.queues[2]);
	pqueue_init(&schedulingInfo.queues[3]);
}