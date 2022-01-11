#pragma once
#include <pthread.h>
#include "err.h"

struct readwrite {
    pthread_mutex_t lock;
    pthread_cond_t readers;
    pthread_cond_t writers;
    int rcount, wcount, rwait, wwait;
    int change;
};

void init(struct readwrite *rw);

void destroy(struct readwrite *rw);

void start_reading(struct readwrite *rw);

void finish_reading(struct readwrite *rw);

void start_writing(struct readwrite *rw);

void finish_writing(struct readwrite *rw);


