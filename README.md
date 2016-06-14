# Signals and Slots

##Features Overview
- Simple signals and slots mechanism
- Specifiable executor (synchronous, asynchronous, strand, thread pooled)
- Constructor specifiable thread safety 
- Thread safety only required for interleaved emission/connection/disconnection

##Executors
Executors determine how a connected slot is invoked on emission. There are 4
different executor modes.

###Synchronous
- Emission occurs synchronously.
- When emit returns, all connected slots have been invoked and returned.
- Preferred for slots with short execution time, where quick emission is
required and/or when it is necessary to know that the function has returned
before proceeding.

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
//  

Multiple slot executors to suit requirements of the signal:
- Synchronized: where return of emission guarantees completion of connected slots
- Asynchronous: emission spawns a detached thread for each connected function
- Strand: Spawns a thread on slot connection which waits for emissions
and processes them in order of arrival
- Thread Pooled: Emission parameters are bound to the function and enqueued, to be
executed by a pool of threads waiting for tasks, each thread is a MPSC queue

Todo:
- Internal documentation
- Thorough benchmark against other signals/slots implementations
