# BSignals

This is a signals and slots library for C++14. 

The intention is to provide a fast thread safe signals/slots mechanism which is easy to use and has no external dependencies. The main differentiating factor between this signals/slots library and others is that it provides the means to specify the type of executor used.

##Table of Contents
- [BSignals](#bsignals)
    - [Table of Contents](#table-of-contents)
    - [Features](#features)
    - [Building and Linking](#building-and-linking)
    - [Usage](#usage)
        - [Simple Usage Example](#simple-usage-example)
        - [Include](#include)
        - [Construct](#construct)
        - [Connect](#connect)
        - [Emit](#emit)
        - [Disconnect](#disconnect)
    - [Executors](#executors)
        - [Synchronous](#synchronous)
        - [Deferred Synchronous](#deferred-synchronous)
        - [Asynchronous](#asynchronous)
        - [Strand](#strand)
        - [Thread Pooled](#thread-pooled)
    - [To Do](#to-do)
    - [Limitations](#limitations)

##Features
- Simple signals and slots mechanism
- Specifiable executor (synchronous, synchronous deferred, asynchronous, strand, thread pooled)
- Constructor specifiable thread safety 
- Thread safety only required for interleaved emission/connection/disconnection

##Building and Linking
To build the default release build, type
```
    make
```
in the base directory.
To build the debug build, type
```    
    make debug
``` 
in the base directory.
Currently only makefile based build is supported - other build systems may be added
in the future. 
For dynamic linking, a shared object is generated in
```
    {BASE_DIRECTORY}/gen/release/lib/dynamic
``` 
For static linking, an archive is generated in
``` 
    {BASE_DIRECTORY}/gen/release/lib/static
``` 
The gtest binary is generated in
``` 
    {BASE_DIRECTORY}/gen/release/test
``` 
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
- Interleaved connects/disconnects are always thread safe
- Interleaved connect/disconnect with emissions are not thread safe

If you cannot make guarantees that the emission and connection/disconnection will
not be interleaved, or you wish to connect/disconnect a slot or slots from within a
slot function, enable thread safety using the constructor:
```
    BSignals::Signal<T1,T2,T...,TN> signalB(true);
```
####Connect
Connected functions must have a return type of void and a signature matching that
of the signal object - i.e. given a Signal object:
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
    signal.connectSlot(BSignals::ExecutorScheme::DEFERRED_SYNCHRONOUS, functionName);
    signal.connectSlot(BSignals::ExecutorScheme::ASYNCHRONOUS, functionName);
    signal.connectSlot(BSignals::ExecutorScheme::STRAND, functionName);
    signal.connectSlot(BSignals::ExecutorScheme::THREAD_POOLED, functionName);
```
To connect a member function, an executor is specified as the first argument, 
the member function name as the second, and the instance reference/pointer as the third.
```
    Foo foo;
    
    //connect by pointer
    int id1 = signal.connectMemberSlot(BSignals::ExecutorScheme::SYNCHRONOUS, &Foo::bar, &foo);

    //connect by reference
    int id2 = signal.connectMemberSlot(BSignals::ExecutorScheme::SYNCHRONOUS, &Foo::bar, foo);
```
####Emit
To emit on a given signal, call emitSignal with the emission parameters.
```
    signal.emitSignal(arg1, arg2);
```
You can also use the () operator overload which invokes the emitSignal function.
```
    signal(arg1, arg2);
```
####Disconnect
To disconnect a slot, call disconnectSlot with the id acquired on connection.
```
    //disconnection/connection should not be interleaved with emission
    //unless the signal was initialized with thread safety enabled
    int id = signal.connectSlot(...);
    signal.disconnectSlot(id);
```
It is also possible to disconnect all connected slots
```
    signal.disconnectAllSlots();
```
##Executors
Executors determine how a connected slot is invoked on emission. There are 5
different executor modes.

####Synchronous
- Emission occurs synchronously.
- When emit returns, all connected slots have been invoked and returned.
- Preferred for slots when
    - they have short execution time
    - quick emission is required
    - Necessary to know that the function has returned before proceeding

####Deferred Synchronous
- Emission occurs synchronously.
- When emit returns, emission has been enqueued for later execution
- Deferred emissions can be invoked using the invokeDeferred function
- Preferred for slots when
    - the time the slots are executed needs to be controlled
    - there is a designated event processor or event loop (such as a GUI render loop)

####Asynchronous
- Emission occurs asynchronously.
- A detached thread is spawned on emission.
- The thread is destroyed when the connected slot returns.
- Preferred for slots when 
    - they have very long, unbounded, or infinite execution time
    - they are independent
    - additional thread overhead is not an issue
    - separate thread required for lifetime of invocation

####Strand
- Emission occurs asynchronously.
- A dedicated thread is spawned on slot connection to wait for new messages
- Emitted parameters are enqueued on the waiting thread to be processed synchronously
- The underlying queue is a (mostly) lock free multi-producer single consumer queue
- Preferred for slots when
    - they have long, bounded, or finite execution time
    - emissions occur in blocks
    - the additional time overhead of creating/destroying a thread for each slot would not be performant
    - emissions need to be processed in order of arrival (FIFO)

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
- Jobs are stolen from other queues when none are available
- Preferred for slots when
    - they have long, bounded, or finite execution time
    - the overhead of creating/destroying a thread for each slot would not be performant
    - the overhead of a waiting thread for each slot (as in the strand executor scheme) is unnecessary
    - connected functions do NOT need to be processed in order of arrival

##Limitations
- Cannot return values from emissions - only void functions/lambdas are accepted
- Requires C++14 for variadic argument <-> tuple unpacking