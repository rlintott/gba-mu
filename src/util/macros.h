#pragma once

#include <iostream>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define NDEBUG 1;
//#define NDEBUGWARN 1;


#ifdef NDEBUG
#define DEBUG(x)
#else
#define DEBUG(x)        \
    do {                \
        std::cout << "INFO: " << x; \
    } while (0)
#endif


#ifdef NDEBUGWARN
#define DEBUGWARN(x)
#else
#define DEBUGWARN(x)        \
    do {                \
        std::cout << "WARN: " << x; \
    } while (0)
#endif
