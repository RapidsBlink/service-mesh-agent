#pragma once

#include <uv.h>
#include <stdlib.h>

typedef struct{
    int value;
    uv_mutex_t lock;
}atomic_int_t;


atomic_int_t* init_atomic_integer(int init_value){
    atomic_int_t* atom = malloc(sizeof(atomic_int_t));
    uv_mutex_init(&(atom->lock));
    atom->value = init_value;
    return atom;
}

int atomic_add(atomic_int_t *atom, int delta){
    int ret = atom->value;
    uv_mutex_lock(&(atom->lock));
    atom->value += delta;
    ret = atom->value;
    uv_mutex_unlock(&(atom->lock));
    return ret;
}