// os345semaphores.c - OS Semaphores
// ***********************************************************************
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// **                                                                   **
// ** The code given here is the basis for the BYU CS345 projects.      **
// ** It comes "as is" and "unwarranted."  As such, when you use part   **
// ** or all of the code, it becomes "yours" and you are responsible to **
// ** understand any algorithm or method presented.  Likewise, any      **
// ** errors or problems become your responsibility to fix.             **
// **                                                                   **
// ** NOTES:                                                            **
// ** -Comments beginning with "// ??" may require some implementation. **
// ** -Tab stops are set at every 3 spaces.                             **
// ** -The function API's in "OS345.h" should not be altered.           **
// **                                                                   **
// **   DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER ** DISCLAMER   **
// ***********************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <time.h>
#include <assert.h>

#include "os345.h"


extern TCB tcb[];                            // task control block
extern int curTask;                            // current task #
extern PQueue rq;

extern int superMode;                        // system mode
extern Semaphore *semaphoreList;            // linked list of active semaphores
extern int scheduler_mode;

// **********************************************************************
// **********************************************************************
// signal semaphore
//
//	if task blocked by semaphore, then clear semaphore and wakeup task
//	else signal semaphore
//
void semSignal(Semaphore *s) {
    int i;
    // assert there is a semaphore and it is a legal type
    assert("semSignal Error" && s && ((s->type == 0) || (s->type == 1)));

    int tid;
    do {
        // check semaphore type
        if (s->type == 0) {
            // binary semaphore
            s->state = 1;                        // nothing waiting, signal

            if ((tid = deQ(s->q, -1)) < 0) {
                // Nothing in the blocked queue, return immediately
                return;
            }
            s->state = 0;  // still blocked b/c we're going to set another as ready

        } else {
            // counting semaphore
            // ?? implement counting semaphore
            // Add a resource.
            if (++s->state > 0) {
                assert(lenQ(s->q) >= 0);
                return;
            }
            tid = deQ(s->q, -1);
            // Should not be possible for our queue to be empty
            assert(tid >= 0);
        }
    } while (tcb[tid].state == S_EXIT);
    tcb[tid].event = 0; // clear event pointer
    tcb[tid].state = S_READY;
    enQ(rq, tid, tcb[tid].priority);

    if (!superMode) swapTask();
} // end semSignal



// **********************************************************************
// **********************************************************************
// wait on semaphore
//
//	if semaphore is signaled, return immediately
//	else block task
//  return whether it was blocked
//
int semWait(Semaphore *s) {
    assert("semWait Error" && s);                                                // assert semaphore
    assert("semWait Error" && ((s->type == 0) || (s->type == 1)));    // assert legal type
    assert("semWait Error" && !superMode);                                // assert user mode

    // check semaphore type
    if (s->type == 0) {
        // binary semaphore
        if (s->state == 1) { // Is the resource available?
            // Resource available, return immediately
            s->state = 0;
            return 0;
        }
    } else {
        // counting semaphore
        // ?? implement counting semaphore
        if (s->state-- >= 0) { // Consume. Is the resource available?
            // Resource available, return immediately
            return 0;
        }
    }
    // Resource not available.

    // Block task.
    tcb[curTask].event = s;
    // Change task state to blocked
    tcb[curTask].state = S_BLOCKED;
    // Move from ready to blocked queue
    deQ(rq, curTask);
    enQ(s->q, curTask, tcb[curTask].priority);

    // Reschedule
    swapTask();
    return 1;
} // end semWait



// **********************************************************************
// **********************************************************************
// try to wait on semaphore
//
//	if semaphore is signaled, return 1
//	else return 0
//
int semTryLock(Semaphore *s) {
    assert("semTryLock Error" && s);                                                // assert semaphore
    assert("semTryLock Error" && ((s->type == 0) || (s->type == 1)));    // assert legal type
    assert("semTryLock Error" && !superMode);                                    // assert user mode

    if (s->state <= 0) {
        return 0;
    }
    s->state--;
    // Assert we're either a counting semaphore or the state is 0 (not negative)
    assert(s->type == 1 || s->state == 0);
    return 1;
} // end semTryLock


// **********************************************************************
// **********************************************************************
// Create a new semaphore.
// Use heap memory (malloc) and link into semaphore list (Semaphores)
// 	name = semaphore name
//		type = binary (0), counting (1)
//		state = initial semaphore state
// Note: memory must be released when the OS exits.
//
Semaphore *createSemaphore(char *name, int type, int state) {
    Semaphore *sem = semaphoreList;
    Semaphore **semLink = &semaphoreList;

    // assert semaphore is binary or counting
    assert("createSemaphore Error" && ((type == 0) || (type == 1)));    // assert type is validate

    // look for duplicate name
    while (sem) {
        if (!strcmp(sem->name, name)) {
            printf("\nSemaphore %s already defined", sem->name);

            // ?? What should be done about duplicate semaphores ??
            // semaphore found - change to new state
            sem->type = type;                    // 0=binary, 1=counting
            sem->state = state;                // initial semaphore state
            sem->taskNum = curTask;            // set parent task #
            return sem;
        }
        // move to next semaphore
        semLink = (Semaphore **) &sem->semLink;
        sem = (Semaphore *) sem->semLink;
    }

    // allocate memory for new semaphore
    sem = (Semaphore *) malloc(sizeof(Semaphore));

    // set semaphore values
    sem->name = (char *) malloc(strlen(name) + 1);
    strcpy(sem->name, name);                // semaphore name
    sem->type = type;                            // 0=binary, 1=counting
    sem->state = state;                        // initial semaphore state
    sem->taskNum = curTask;                    // set parent task #
    sem->q = initQ();

    // prepend to semaphore list
    sem->semLink = (struct semaphore *) semaphoreList;
    semaphoreList = sem;                        // link into semaphore list
    return sem;                                    // return semaphore pointer
} // end createSemaphore



// **********************************************************************
// **********************************************************************
// Delete semaphore and free its resources
//
bool deleteSemaphore(Semaphore **semaphore) {
    Semaphore *sem = semaphoreList;
    Semaphore **semLink = &semaphoreList;

    // assert there is a semaphore
    assert("deleteSemaphore Error" && *semaphore);

    // look for semaphore
    while (sem) {
        if (sem == *semaphore) {
            // semaphore found, delete from list, release memory
            *semLink = (Semaphore *) sem->semLink;

            // free the name array before freeing semaphore
            printf("\ndeleteSemaphore(%s)", sem->name);

            // ?? free all semaphore memory
            // ?? What should you do if there are tasks in this
            //    semaphores blocked queue????
            if (sem->q) {
                delQ(&sem->q);
            }
            free(sem->name);
            free(sem);

            return TRUE;
        }
        // move to next semaphore
        semLink = (Semaphore **) &sem->semLink;
        sem = (Semaphore *) sem->semLink;
    }

    // could not delete
    return FALSE;
} // end deleteSemaphore
