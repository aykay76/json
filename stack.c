#include <stdlib.h>
#include "include/stack.h"
#include "include/value.h"

statestack_t* newstack(enum State root)
{
    statestack_t* stack = (statestack_t*)malloc(sizeof(statestack_t));

    stack->prev = 0;
    stack->state = root;

    return stack;
}

statestack_t* pushstack(statestack_t* stack, enum State item)
{
    statestack_t* newstack = (statestack_t*)malloc(sizeof(statestack_t));
    
    newstack->prev = stack;
    newstack->state = item;

    return newstack;
}

statestack_t* popstack(statestack_t* stack)
{
    statestack_t* prev = stack->prev;
    free(stack);
    return stack->prev;
}

void deletestack(statestack_t* stack)
{
    statestack_t* prev = stack->prev;
    while (prev)
    {
        free(stack);
        prev = prev->prev;
    }
}