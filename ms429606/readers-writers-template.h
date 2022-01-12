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

void rw_init(struct readwrite* rw);

void rw_destroy(struct readwrite* rw);

void rw_reader_preliminary_protocol(struct readwrite* rw);

void rw_reader_final_protocol(struct readwrite* rw);

void rw_writer_preliminary_protocol(struct readwrite* rw);

void rw_writer_final_protocol(struct readwrite* rw);


