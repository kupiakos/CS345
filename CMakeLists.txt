cmake_minimum_required(VERSION 3.6)
project(Shell C)

set(SOURCE_FILES
        os345.c
        os345.h
        os345config.h
        os345fat.c
        os345fat.h
        os345interrupts.c
        os345lc3.c
        os345lc3.h
        os345mmu.c
        os345p1.c
        os345p2.c
        os345p3.c
        os345p4.c
        os345p5.c
        os345p6.c
        os345park.c
        os345park.h
        os345semaphores.c
        os345signals.c
        os345signals.h
        os345tasks.c
        pqueue.c pqueue.h utils.c utils.h)
add_executable(Shell ${SOURCE_FILES})