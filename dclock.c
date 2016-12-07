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
    SWAP;
    DClock clock = malloc(sizeof(struct s_DClock));
    SWAP;
    assert(clock);

    clock->name = malloc(strlen(name) + 1);
    SWAP;
    assert(clock->name);
    SWAP;
    strcpy(clock->name, name);
    SWAP;

    strcpy(mutexNameBuf, DC_MUTEX_HEADER);
    SWAP;
    strcat(mutexNameBuf, name);
    SWAP;
    clock->mutex = createSemaphore(mutexNameBuf, BINARY, 1);
    SWAP;
    clock->events = NULL;
    SWAP;
    return clock;
    SWAP;
}

void tickDClock(DClock clock, int ticks) {
    assert(clock);
    SEM_WAIT(clock->mutex);
    SWAP;
    tickDClock_safe(clock, ticks);
    SWAP;
    SEM_SIGNAL(clock->mutex);
    SWAP;
}

void tickDClock_safe(DClock clock, int ticks) {
    assert(clock);
    assert(ticks >= 0);
    if (ticks == 0 || !clock->events)
        return;
    SWAP;
    DClockEvent head = clock->events;
    SWAP;
    head->delta -= ticks;
    SWAP;
    if (head->delta <= 0) {
        clock->events = head->next;
        SWAP;
        SEM_SIGNAL(head->event);
        SWAP;
        // propagate any remaining ticks left
        tickDClock_safe(clock, -head->delta);
        SWAP;
        free(head);
        SWAP;
    }
}

void insertDClock(DClock clock, int delta, Semaphore *event) {
    assert(clock);
    assert(event->q);
    SEM_WAIT(clock->mutex);

    DClockEvent *next;
    SWAP;
    next = &clock->events;
    SWAP;
    // Find what event we belong after
    while (*next && delta - (*next)->delta > 0) {
        assert((*next)->delta >= 0);
        SWAP;
        delta -= (*next)->delta;
        SWAP;
        next = &(*next)->next;
        SWAP;
    }
    assert(delta >= 0);
    // next now points to the DClockEvent we will insert at
    // delta is the amount left in the clock
    DClockEvent e = malloc(sizeof(struct s_DClockEvent));
    SWAP;
    assert(e);

    e->next = *next;
    SWAP;
    e->event = event;
    SWAP;
    e->delta = delta;
    SWAP;
    // This will automatically reassign clock->events if e is the new head event
    *next = e;
    SWAP;
    if (e->delta > 0) {
        // The event after e needs to be adjusted to now be a delta of e
        if (e->next) {
            e->next->delta -= delta;
            assert(e->next->delta >= 0);
        }
        SWAP;
    }

    SEM_SIGNAL(clock->mutex);
    SWAP;
}

void printDClock(DClock clock, bool showAbsolute) {
    assert(clock);
    SWAP;
    SEM_WAIT(clock->mutex);
    SWAP;
    printf("\nDelta Clock %s:\n", clock->name);
    SWAP;
    int delta = 0;
    SWAP;
    for (DClockEvent e = clock->events; e; e = e->next) {
        if (!showAbsolute) {
            delta = 0;
            SWAP;
        }
        delta += e->delta;
        SWAP;
        printf("%03d - %s\n", delta, e->event->name);
        SWAP;
    }
    SEM_SIGNAL(clock->mutex);
    SWAP;
}

void delDClock(DClock *clock) {
    assert(clock);
    SWAP;
    assert(*clock);
    SWAP;
    SEM_WAIT((*clock)->mutex);
    SWAP;
    for (DClockEvent e = (*clock)->events, next; e; e = next) {
        next = e->next;
        SWAP;
        free(e);
        SWAP;
    }
    (*clock)->events = NULL;
    SWAP;
    free((*clock)->name);
    SWAP;
    deleteSemaphore(&(*clock)->mutex);
    SWAP;
    (*clock)->mutex = NULL;
    SWAP;
    free(*clock);
    SWAP;
    *clock = NULL;
    SWAP;
}
