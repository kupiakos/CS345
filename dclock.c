//
// Created by kevin on 12/6/16.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "dclock.h"

#define DC_MUTEX_HEADER "DClockMutex-"

typedef struct s_DClockEvent {
    int delta;
    Semaphore *event;
    struct s_DClockEvent *next;
} *DClockEvent;

struct s_DClock {
    char *name;
    Semaphore *mutex;
    DClockEvent events;
};

static void tickDClock_safe(DClock clock, int ticks);

DClock initDClock(const char *name) {
    char mutexNameBuf[strlen(name) + sizeof(DC_MUTEX_HEADER)];
    DClock clock = malloc(sizeof(struct s_DClock));
    assert(clock);

    clock->name = malloc(strlen(name) + 1);
    assert(clock->name);
    strcpy(clock->name, name);

    strcpy(mutexNameBuf, DC_MUTEX_HEADER);
    strcat(mutexNameBuf, name);
    clock->mutex = createSemaphore(mutexNameBuf, BINARY, 1);
    clock->events = NULL;
    return clock;
}

void tickDClock(DClock clock, int ticks) {
    assert(clock);
    SEM_WAIT(clock->mutex);
    tickDClock_safe(clock, ticks);
    SEM_SIGNAL(clock->mutex);
}

void tickDClock_safe(DClock clock, int ticks) {
    assert(clock);
    assert(ticks >= 0);
    if (ticks == 0 || !clock->events)
        return;
    DClockEvent head = clock->events;
    head->delta -= ticks;
    if (head->delta <= 0) {
        clock->events = head->next;
        SEM_SIGNAL(head->event);
        // propagate any remaining ticks left
        tickDClock_safe(clock, -head->delta);
        free(head);
    }
}

void insertDClock(DClock clock, int delta, Semaphore *event) {
    assert(clock);
    SEM_WAIT(clock->mutex);

    DClockEvent *next;
    next = &clock->events;
    // Find what event we belong after
    while (*next && delta - (*next)->delta > 0) {
        delta -= (*next)->delta;
        next = &(*next)->next;
    }
    assert(delta >= 0);
    // next now points to the DClockEvent we will insert at
    // delta is the amount left in the clock
    DClockEvent e = malloc(sizeof(struct s_DClockEvent));
    assert(e);

    e->next = *next;
    e->event = event;
    e->delta = delta;
    // This will automatically reassign clock->events if e is the new head event
    *next = e;
    if (e->delta > 0) {
        // Everything later than e now needs to have its delta decremented
        for (e = e->next; e; e = e->next) {
            e->delta -= delta;
        }
    }

    SEM_SIGNAL(clock->mutex);
}

void printDClock(DClock clock, bool showAbsolute) {
    assert(clock);
    SEM_WAIT(clock->mutex);
    printf("\nDelta Clock %s:\n", clock->name);
    int delta = 0;
    for (DClockEvent e = clock->events; e; e = e->next) {
        if (!showAbsolute) {
            delta = 0;
        }
        delta += e->delta;
        printf("%03d - %s\n", e->delta, e->event->name);
    }
    SEM_SIGNAL(clock->mutex);
}

void delDClock(DClock *clock) {
    assert(clock);
    assert(*clock);
    SEM_WAIT((*clock)->mutex);
    for (DClockEvent e = (*clock)->events, next; e; e = next) {
        next = e->next;
        free(e);
    }
    (*clock)->events = NULL;
    free((*clock)->name);
    deleteSemaphore(&(*clock)->mutex);
}
