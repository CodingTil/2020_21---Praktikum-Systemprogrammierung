/*! \file
 *  \brief Scheduling library for the OS.
 *
 *  Contains the scheduling strategies.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#ifndef _OS_SCHEDULING_STRATEGIES_H
#define _OS_SCHEDULING_STRATEGIES_H

#include "os_scheduler.h"
#include "defines.h"

//! Structure used to store specific scheduling informations such as a time slice
typedef struct {
	uint8_t timeSlice; // quantum
	Age age[MAX_NUMBER_OF_PROCESSES];
	ProcessQueue queues[4];
	uint8_t timeslices[4];
	uint8_t remainingts[MAX_NUMBER_OF_PROCESSES];
} SchedulingInformation;


//! Structure for Multilevel-Feedback Queue
typedef struct {
	ProcessID data[MAX_NUMBER_OF_PROCESSES];
	uint8_t head;
	uint8_t tail;
	uint8_t size;
} ProcessQueue; 

//! Used to reset the SchedulingInfo for one process
void os_resetProcessSchedulingInformation(ProcessID id);

//! Used to reset the SchedulingInfo for a strategy
void os_resetSchedulingInformation(SchedulingStrategy strategy);

//! Even strategy
ProcessID os_Scheduler_Even(Process const processes[], ProcessID current);

//! Random strategy
ProcessID os_Scheduler_Random(Process const processes[], ProcessID current);

//! RoundRobin strategy
ProcessID os_Scheduler_RoundRobin(Process const processes[], ProcessID current);

//! InactiveAging strategy
ProcessID os_Scheduler_InactiveAging(Process const processes[], ProcessID current);

//! RunToCompletion strategy
ProcessID os_Scheduler_RunToCompletion(Process const processes[], ProcessID current);

//! Initializes ring buffer
void pqueue_init(ProcessQueue *queue);

//! Resets ring buffer
void pqueue_reset(ProcessQueue *queue);

//! Checks if there are any PIDs stored
uint8_t pqueue_hasNext(ProcessQueue *queue);

//! Functions for reading and writing
ProcessID pqueue_getFirst(ProcessQueue *queue);

void pqueue_dropFirst(ProcessQueue *queue);

void pqueue_append(ProcessQueue *queue, ProcessID pid);

ProcessQueue* MLFQ_getQueue(uint8_t queueID);

void os_initSchedulingInformation();

ProcessID os_Scheduler_MLFQ(Process const processes[], ProcessID current);

void MLFQ_removePID(ProcessID pid);

uint8_t MLFQ_getDefaultTimeslice(uint8_t queueID);

uint8_t MLFQ_MapToQueue(Priority prio);

#endif
