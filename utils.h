//
// Created by kevin on 11/9/16.
//

#ifndef SHELL_UTILS_H
#define SHELL_UTILS_H

#include <stdbool.h>


void swapSleep(float secs);

typedef unsigned char *BitField;

// TODO: Implement eventually
// size is number of bits, not bytes
BitField initBits(size_t size);

bool isBitSet(BitField b);
void setBit(BitField b, size_t i, bool set);

// Return the result value of the bit
bool toggleBit(BitField b, size_t i);

void delBits(BitField b);


#endif //SHELL_UTILS_H
