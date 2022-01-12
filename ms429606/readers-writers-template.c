/* Author Mikołaj Szkaradek */
#include "readers-writers-template.h"

// Initialize rw.
void rw_init(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_init(&rw->lock, 0)) != 0)
        syserr("Error in mutex rw_init.\n", err);
    if ((err = pthread_cond_init(&rw->readers, 0)) != 0)
        syserr("Error in cond rw_init.\n", err);
    if ((err = pthread_cond_init(&rw->writers, 0)) != 0)
        syserr("Error in cond rw_init.\n", err);
    rw->rcount = 0;
    rw->wcount = 0;
    rw->rwait = 0;
    rw->wwait = 0;
    rw->change = 0;
}

// Destroy rw elements from pthread.
void rw_destroy(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_destroy(&rw->lock)) != 0)
        syserr("Error in mutex rw_destroy.\n", err);
    if ((err = pthread_cond_destroy(&rw->writers)) != 0)
        syserr("Error in cond rw_destroy.\n", err);
    if ((err = pthread_cond_destroy(&rw->readers)) != 0)
        syserr("Error in cond rw_destroy.\n", err);
}

void rw_reader_preliminary_protocol(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_lock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);

    while (rw->wcount > 0 || (rw->wwait > 0 && rw->change == 1)) {
        rw->rwait++;
        if ((err = pthread_cond_wait(&rw->readers, &rw->lock)) != 0)
            syserr("Error in wait(readers).\n", err);
        rw->rwait--;
    }
    rw->rcount++;

    if (rw->rwait > 0) {
        if ((err = pthread_cond_signal(&rw->readers)) != 0)
            syserr( "Error in broadcast\n.", err);
    }

    if ((err = pthread_mutex_unlock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);
}

void rw_reader_final_protocol(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_lock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);

    rw->rcount--;

    if (rw->rcount == 0 && rw->wwait > 0) {
        if ((err = pthread_cond_signal(&rw->writers)) != 0)
            syserr("Error in signal(writers).\n", err);
    }

    if ((err = pthread_mutex_unlock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);
}

void rw_writer_preliminary_protocol(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_lock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);
    rw->change = 1;
    while(rw->rcount + rw->wcount > 0) {
        rw->wwait++;
        if ((err = pthread_cond_wait(&rw->writers, &rw->lock)) != 0)
            syserr("Error in wait(writers).\n", err);
        rw->wwait--;
    }
    rw->wcount++;

    if ((err = pthread_mutex_unlock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);
}

void rw_writer_final_protocol(struct readwrite* rw) {
    int err;

    if ((err = pthread_mutex_lock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);

    rw->wcount--;

    if (rw->rwait > 0) {
        rw->change = 0;
        if ((err = pthread_cond_signal(&rw->readers)) != 0)
            syserr("Error in signal(readers).\n", err);
    }
    else if (rw->wwait > 0) {
        if ((err = pthread_cond_signal(&rw->writers)) != 0)
            syserr("Error in signal(writers).\n", err);
    }

    if ((err = pthread_mutex_unlock(&rw->lock)) != 0)
        syserr("Error in P(mutex).\n", err);
}
