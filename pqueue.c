//
// Created by kevin on 11/8/16.
//

#include "pqueue.h"
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include "os345.h"

union PQueue_u {
    size_t size;
    struct {
        Priority priority;
        TID tid;
    };
};


PQueue initQ() {
    PQueue q = malloc((MAX_TASKS + 1) * sizeof(union PQueue_u));
    q[0].size = 0;
}

int enQ(PQueue q, TID tid, Priority p) {
    PQueue entry = (q + q[0].size + 1);
    entry->tid = tid;
    entry->priority = p;
    return tid;
}

static void delElement(PQueue q, int i) {
    assert(q[0].size > 0);
    memmove(q + i, q + i + 1, (q[0].size - i) * sizeof(*q));
    --q[0].size;
}

static int tidElement(PQueue q, TID tid) {
    for (int i = 1; i <= q[0].size; ++i) {
        if (q[i].tid == tid) {
            return i;
        }
    }
    return -1;
}

static int highestElement(PQueue q) {
    int highest = -1, highestIndex = -1;

    for (int i = 1; i <= q[0].size; ++i) {
        if (q[i].priority > highest) {
            highest = q[i].priority;
            highestIndex = i;
        }
    }
    return highestIndex;
}

int deQ(PQueue q, TID tid) {
    int idx;
    if (tid == -1) {
        idx = highestElement(q);
    } else {
        idx = tidElement(q, tid);
    }
    if (idx == -1) {
        return -1;
    }
    int result = q[idx].tid;
    delElement(q, idx);
    return result;
}

void delQ(PQueue *q) {
    free(*q);
    *q = NULL;
}

size_t lenQ(PQueue q) {
    return q[0].size;
}
