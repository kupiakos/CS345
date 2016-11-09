//
// Created by kevin on 11/8/16.
//

#ifndef SHELL_PQUEUE_H
#define SHELL_PQUEUE_H

typedef int TID;
typedef int Priority;
typedef union PQueue_u *PQueue;

PQueue initQ();
int enQ(PQueue q, TID tid, Priority p);
int deQ(PQueue q, TID tid);
void delQ(PQueue *q);

#endif //SHELL_PQUEUE_H
