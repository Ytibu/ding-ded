#ifndef __HEAD_H__
#define __HEAD_H__

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>


void scc(int code);
void *scp(void *ptr);

#define NULL_CHECK(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            printf("Error: %s | File: %s | Function: %s | Line: %d\n", msg, __FILE__, __func__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


#define ZERO_CHECK(code, msg) \
    do { \
        if ((code) == 0) { \
            printf("Error: %s | File: %s | Function: %s | Line: %d\n", msg, __FILE__, __func__, __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

#define ERROR(msg) \
    do { \
        printf("\033[1;31mError: %s | File: %s | Function: %s | Line: %d\033[0m\n", msg, __FILE__, __func__, __LINE__); \
        exit(EXIT_FAILURE); \
    } while(0)

#define INFO(msg) \
    do { \
        printf("\033[1;34mInfo: %s | File: %s | Function: %s | Line: %d\033[0m\n", msg, __FILE__, __func__, __LINE__); \
    } while(0)

#endif // end of __HEAD_H__