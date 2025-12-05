// debugger.h
#pragma once

#include <cstdio>

#define DEBUGGER

#ifdef DEBUGGER
#define LOGFL                                        \
    do {                                             \
        printf("\nLOG %s:%d\n", __FILE__, __LINE__); \
    } while (0);
#else
#define LOGFL
#endif
