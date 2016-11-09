//
// Created by kevin on 11/9/16.
//

#include <time.h>
#include "utils.h"
#include "os345.h"

void swapSleep(float secs) {
    clock_t start = clock();
    while ((clock() - start) / CLOCKS_PER_SEC < secs) {
        swapTask();
    }
}
