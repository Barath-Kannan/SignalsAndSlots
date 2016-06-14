# BSignals

This is a signals and slots library for C++14. 

The intention is to provide a fast thread safe signals/slots mechanism which is 
easy to use, has no external dependencies, and allows specification of different
executors for additional flexibility.

##Table of Contents
- [BSignals](#bsignals)
    - [Table of Contents](#table of contents)
    - [Features](#features)
    - [Building and Linking](#building and linking)
    - [Usage]
        - [Simple Usage Example]
        - [Include]
        - [Construct]
        - [Connect]
        - [Emit]
        - [Disconnect]
    - [Executors]
        - [Synchronous]
        - [Asynchronous]
        - [Strand]
        - [Thread Pooled]
    - [To Do]
    - [Limitations]

##Features
- Simple signals and slots mechanism
- Specifiable executor (synchronous, asynchronous, strand, thread pooled)
- Constructor specifiable thread safety 
- Thread safety only required for interleaved emission/connection/disconnection

##Building and Linking
To build the default release build, type
    
    make
    
in the base directory.
To build the debug build, type
    
    make debug
    
in the base directory.
Currently only makefile based build is supported - other build systems may be added
in the future. 
For dynamic linking, a shared object is generated in
    
    {BASE_DIRECTORY}/gen/release/lib/dynamic
    
For static linking, an archive is generated in
    
    {BASE_DIRECTORY}/gen/release/lib/static
    
The gtest binary is generated in
    
    {BASE_DIRECTORY}/gen/release/test
    
##Usage

Below is a summary of how to use the Signal class.

For more examples, see SignalTest.cpp in the test directory.

####Simple Usage Example
```
    #include <iostream>
    #include <BSignals/Signal.hpp>

    int main(int argc, char *argv[]){
        BSignals::Signal<int, int> testSignal;
        int id = testSignal.connectSlot(BSignals::ExecutorScheme::SYNCHRONOUS, 
            [](int x, int y){
                std::cout << x << " + " << y << " = " << x + y << std::endl;
            });
        testSignal.emitSignal(1, 2);
        testSignal.disconnectSlot(id);
        return 0;
    }
```
####Include
```
    #include <BSignals/Signal.hpp>
```
####Construct
```
    //Default constructor
    BSignals::Signal<T1,T2,T...,TN> signalA; // T1..TN are the types required by emission
```
- Full thread safety is disabled by default for performance reasons
- Interleaved emissions are always thread safe
- Interleaved connects/disconnects always are thread safe
- Interleaved connect/disconnect with emissions are not thread safe

If you cannot make guarantees that the emission and connection/disconnection will
not be interleaved, enable thread safety using the constructor:
```
    BSignals::Signal<T1,T2,T...,TN> signalB(true);
```
To specify the maximum number of asynchronous threads a signal can spawn, call
 the constructor with an unsigned integer, as below.
```
    BSignals::Signal<T1,T2,T...,TN> signalC(512); //Default is 1024
```
To specify both thread safety and the number of asynchronous threads, call the 
constructor with a boolean and unsigned integer, as below.
```
    BSignals::Signal<T1,T2,T...,TN> signalD(true, 512);
```
####Connect
Connected functions must have a return type of void and a signature matching that
of the signal object. Given a Signal object:
```
    BSignals::Signal<int, int> signalInt;
```
Connected slots must must have the signature:
```
    void functionName(int a, int b);
```
To connect a function, an executor is specified as the first argument, and the
function name as the second.
```
    //id can be used later for disconnection
    int id = signal.connectSlot(BSignals::ExecutorScheme::SYNCHRONOUS, functionName);
```
Alternative executor schemes can also be specified (see Executor section below 
for more details).
```
    signal.connectSlot(BSignals::ExecutorScheme::ASYNCHRONOUS, functionName);
    signal.connectSlot(BSignals::ExecutorScheme::STRAND, functionName);
    signal.connectSlot(BSignals::ExecutorScheme::THREAD_POOLED, functionName);
```
To connect a member function, an executor is specified as the first argument, 
the member function name as the second, and the instance reference/pointer as the third.
```
    Foo foo;
    int id = signal.connectMemberSlot(BSignals::ExecutorScheme::SYNCHRONOUS, &Foo::bar, foo);
```
####Emit
To emit on a given signal, call emitSignal with the emission parameters.
```
    signal.emitSignal(arg1, arg2);
```
####Disconnect
To disconnect a slot, call disconnectSlot with the id acquired on connection.
```
    //disconnection/connection should not be interleaved with emission unless the
    //signal was initialized with thread safety enabled
    int id = signal.connectSlot(...);
    signal.disconnectSlot(id);
```
It is also possible to disconnect all connected slots
```
    signal.disconnectAllSlots();
```
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

##To Do
- Dynamically scaling thread pool (based on business)
- Weighted round robin in thread pool (based on remaining tasks in each queue)
- Add executor for signal localised thread pool, as opposed to current global
thread pool
- Benchmark emit and connected function completion time against other 
signals/slots implementations

##Limitations
- Requires a C++14 compiler (for shared mutex)
- Cannot return values from emissions - only void functions/lambdas are accepted
