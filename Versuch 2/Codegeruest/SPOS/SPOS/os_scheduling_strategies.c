#include "os_scheduling_strategies.h"
#include "defines.h"

#include <stdlib.h>

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 * \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    // This is a presence task
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
    // This is a presence task
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
		if(processes[i % MAX_NUMBER_OF_PROCESSES]->state == OS_PS_READY && i % MAX_NUMBER_OF_PROCESSES != 0) return i % MAX_NUMBER_OF_PROCESSES;
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
		if(processes[i]->state == OS_PS_READY) {
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
    if(processes[current]->priority == 0) return os_Scheduler_Even(processes, current);
	processes[current]->priority -= 1;
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
	// Increasing age + Sorting by Insertion Sort
	/*
	 * This code is shit.
	 * - Author
	 */
	uint8_t k, j;
	uint8_t ages[MAX_NUMBER_OF_PROCESSES - 1];
	uint8_t indices[MAX_NUMBER_OF_PROCESSES - 1];
    for(uint8_t i = 1; i < MAX_NUMBER_OF_PROCESSES; i++) {
		if(processes[i]->state != OS_PS_UNUSED && i != current) {
			processes[i]->age += processes[i]->priority;
			k = processes[i]->age;
		}else if(i == current) {
			k = processes[i]->age;
		}else {
			k = 0;
		}
		j = i - 2;
		while(j >= 0 && ages[j] > k) {
			ages[j + 1] == ages[j];
			indices[j + 1] == indices[j];
			j--;
		}
		ages[j + 1] = k;
		indices[j + 1] = i;
	}
	if(ages[0] > ages[1]) {
		processes[indices[0]]->age = processes[indices[0]]->priority;
		return indices[0];
	}else {
		uint8_t max_prio = processes[indices[0]]->priority;
		uint8_t max_age = ages[0];
		uint8_t max_index_index;
		for(uint8_t i = MAX_NUMBER_OF_PROCESSES - 2; i > 0; i++) {
			if(ages[i] == max_age && processes[indices[i]]->priority > max_age) {
				max_age = processes[indices[i]]->priority;
				max_index_index = i + 1;
			}
		}
		if(max_index_index == 0) return 0; // Leerlauf
		return indices[max_index_index - 1];
	}
	
	
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
    if(processes[current]->state == OS_PS_UNUSED) return os_Scheduler_Even(processes, current);
	return current;
}
