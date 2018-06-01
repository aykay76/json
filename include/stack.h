#ifndef _stack_h
#define _stack_h

#include "value.h"

typedef struct _statestack
{
    enum State state;
    struct _statestack* prev;
} statestack_t;

statestack_t*newstack(enum State root);

statestack_t* pushstack(statestack_t *stack, enum State item);

statestack_t* popstack(statestack_t *stack);

void deletestack(statestack_t *stack);

#endif