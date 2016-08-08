/* 
 * File:   ExecutorScheme.h
 * Author: Barath Kannan
 *
 * Created on 8 August 2016, 9:25 PM
 */

#ifndef BSIGNALS_EXECUTORSCHEME_H
#define BSIGNALS_EXECUTORSCHEME_H

namespace BSignals{

//These executors determine how message emission to a slot occurs
    // SYNCHRONOUS:
    // Emission occurs synchronously.
    // When emit returns, all connected slots have been invoked and returned.
    // This method is preferred when connected functions have short execution
    // time, quick emission is required, and/or when it is necessary to know 
    // that the function has returned before proceeding.

    // ASYNCHRONOUS:
    // Emission occurs asynchronously. A detached thread is spawned on emission.
    // When emit returns, the thread has been spawned. The thread automatically
    // destructs when the connected function returns.
    // This method is recommended when connected functions have long execution
    // time and are independent.

    // STRAND:
    // Emission occurs asynchronously. 
    // On connection a dedicated thread (per slot) is spawned to wait for new messages.
    // Emitted parameters are bound to the mapped function and enqueued on the 
    // waiting thread. These messages are then processed synchronously in the
    // spawned thread.
    // This method is recommended when connected functions have longer execution
    // time, the overhead of creating/destroying a thread for each slot would be
    // unperformant, and/or connected functions need to be processed in order 
    // of arrival (FIFO).

    // THREAD POOLED:
    // Emission occurs asynchronously. 
    // On connection, if it is the first thread pooled function by any signal, 
    // the thread pool is initialized with 8 threads, all listening for queued
    // emissions. The number of threads in the pool is not currently run-time
    // configurable but may be in the future.
    // Emitted parameters are bound to the mapped function and enqueued on the 
    // one of the waiting threads. These messages are then processed when the 
    // relevant queue is consumed by the mapped thread pool.
    // This method is recommended when connected functions have longer execution
    // time, the overhead of creating/destroying a thread for each slot would be
    // unperformant, the overhead of a waiting thread for each slot is 
    // unnecessary, and/or connected functions do NOT need to be processed in
    // order of arrival.
    
    // DEFERRED_SYNCHRONOUS:
    // Emissions are queued up to be manually invoked through the invokeDeferred function
//
    
enum class ExecutorScheme {
    SYNCHRONOUS,
    DEFERRED_SYNCHRONOUS,
    ASYNCHRONOUS,
    STRAND,
    THREAD_POOLED
};

}

#endif /* BSIGNALS_EXECUTORSCHEME_H */

