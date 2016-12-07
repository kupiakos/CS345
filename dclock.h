//
// Created by kevin on 12/6/16.
//

#ifndef SHELL_DCLOCK_H
#define SHELL_DCLOCK_H

#include "os345.h"

typedef struct s_DClock *DClock;

DClock initDClock(const char *name);
void tickDClock(DClock clock, int ticks);
void insertDClock(DClock clock, int delta, Semaphore *event);
void printDClock(DClock clock, bool showAbsolute);
void delDClock(DClock *clock);

#endif //SHELL_DCLOCK_C_H
