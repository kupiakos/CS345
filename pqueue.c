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
    assert(q);
    PQueue entry = (q + ++q[0].size);
    entry->tid = tid;
    entry->priority = p;
    return tid;
}

static void delElement(PQueue q, int i) {
    assert(q);
    assert(q[0].size > 0);
    memmove(q + i, q + i + 1, (q[0].size - i) * sizeof(*q));
    --q[0].size;
}

static int tidElement(PQueue q, TID tid) {
    assert(q);
    for (int i = 1; i <= q[0].size; ++i) {
        if (q[i].tid == tid) {
            return i;
        }
    }
    return -1;
}

static int highestElement(PQueue q) {
    assert(q);
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
    assert(q);
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
    assert(q && *q);
    free(*q);
    *q = NULL;
}

size_t lenQ(PQueue q) {
    assert(q);
    return q[0].size;
}

int compPriority(const void *i, const void *j) {
    Priority pi = ((PQueue) i)->priority;
    Priority pj = ((PQueue) j)->priority;
    return pj - pi;
}

size_t listQ(PQueue q, TID *tids) {
    assert(q);
    qsort(q + 1, q[0].size, sizeof(*q), &compPriority);
    for (int i = 1; i <= q[0].size; ++i) {
        tids[i - 1] = q[i].tid;
    }
    return q[0].size;
}
