# Signals and Slots

##Features Overview
- Simple signals and slots mechanism
- Specifiable executor (synchronous, asynchronous, strand, thread pooled)
- Constructor specifiable thread safety 
- Thread safety only required for interleaved emission/connection/disconnection

##Executors
Executors determine how a connected slot is invoked on emission. There are 4
different executor modes.

####Synchronous
- Emission occurs synchronously.
- When emit returns, all connected slots have been invoked and returned.
- Preferred for slots with short execution time, where quick emission is
required and/or when it is necessary to know that the function has returned
before proceeding.

####Asynchronous
- Emission occurs asynchronously.
- A detached thread is spawned on emission.
- The thread is destroyed when the connected slot returns.
- This method is recommended when connected functions have long execution time
and are independent, and/or a dedicated thread is required on emission

####Strand
- Emission occurs asynchronously.
- A dedicated thread is spawned on slot connection to wait for new messages
- Emitted parameters are enqueued on the waiting thread to be processed synchronously
- The underlying queue is a (mostly) lock free multi-producer single consumer queue
- This method is recommended when connected functions have longer execution
time, emissions occur in blocks, the overhead of creating/destroying a thread
for each slot would not be performant, and/or connected functions need to be
processed in order of arrival (FIFO).

####Thread Pooled
- Emission occurs asynchronously. 
- On connection, if it is the first thread pooled function slotted by any signal, 
the thread pool is initialized with 32 threads, all listening for queued emissions.
The number of threads in the pool is not currently run-time configurable but may
be in the future
- Emitted parameters are bound to the mapped function and enqueued on one of the
waiting thread queues
- The underlying structure is an array of multi-producer single consumer queues,
with tasks allocated to each queue using round robin scheduling
- This method is recommended when connected functions have longer execution time,
the overhead of creating/destroying a thread for each slot would not be performant,
the overhead of a waiting thread for each slot (as in the strand executor scheme)
is unnecessary, and/or connected functions do NOT need to be processed in order
of arrival.  

##Usage



##To do
- Dynamically scaling thread pool (based on business)
- Weighted round robin in thread pool (based on remaining tasks in each queue)
- Thorough benchmark against other signals/slots implementations
