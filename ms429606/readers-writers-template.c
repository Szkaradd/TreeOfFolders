/* Author Mikołaj Szkaradek */
#include "readers-writers-template.h"
#include <stdlib.h>
#include <string.h>

// Initialize rw.
void rw_init(struct readwrite* rw) {
    CHECK(pthread_mutex_init(&rw->lock, 0));
    CHECK(pthread_cond_init(&rw->readers, 0));
    CHECK(pthread_cond_init(&rw->writers, 0));

    rw->rcount = 0;
    rw->wcount = 0;
    rw->rwait = 0;
    rw->wwait = 0;
    rw->change = false;
}

// Destroy rw elements from pthread.
void rw_destroy(struct readwrite* rw) {
    CHECK(pthread_mutex_destroy(&rw->lock));
    CHECK(pthread_cond_destroy(&rw->writers));
    CHECK(pthread_cond_destroy(&rw->readers));
}

void rw_reader_preliminary_protocol(struct readwrite* rw) {
    CHECK(pthread_mutex_lock(&rw->lock));

    while (rw->wcount > 0 || (rw->wwait > 0 && rw->change == true)) {
        rw->rwait++;
        CHECK(pthread_cond_wait(&rw->readers, &rw->lock));
        rw->rwait--;
    }
    rw->rcount++;

    if (rw->rwait > 0) {
        CHECK(pthread_cond_signal(&rw->readers));
    }

    CHECK(pthread_mutex_unlock(&rw->lock));
}

void rw_reader_final_protocol(struct readwrite* rw) {
    CHECK(pthread_mutex_lock(&rw->lock));

    rw->rcount--;

    if (rw->rcount == 0 && rw->wwait > 0) {
        CHECK(pthread_cond_signal(&rw->writers));
    }

    CHECK(pthread_mutex_unlock(&rw->lock));
}

void rw_writer_preliminary_protocol(struct readwrite* rw) {
    CHECK(pthread_mutex_lock(&rw->lock));

    rw->change = true;
    while(rw->rcount + rw->wcount > 0) {
        rw->wwait++;
        CHECK(pthread_cond_wait(&rw->writers, &rw->lock));
        rw->wwait--;
    }
    rw->wcount++;

    CHECK(pthread_mutex_unlock(&rw->lock));
}

void rw_writer_final_protocol(struct readwrite* rw) {
    CHECK(pthread_mutex_lock(&rw->lock));

    rw->wcount--;

    if (rw->rwait > 0) {
        rw->change = false;
        CHECK(pthread_cond_signal(&rw->readers));
    }
    else if (rw->wwait > 0) {
        CHECK(pthread_cond_signal(&rw->writers));
    }

    CHECK(pthread_mutex_unlock(&rw->lock));
}
