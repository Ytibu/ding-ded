#include "head.h"

void scc(int code)
{
    if (code < 0)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

void *scp(void *ptr)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    return ptr;
}