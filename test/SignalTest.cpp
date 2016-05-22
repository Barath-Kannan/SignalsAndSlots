#include "SignalTest.h"
#include <iostream>
#include <list>
#include <vector>
#include <thread>
#include <atomic>

#include "BasicTimer.h"
#include "BSignals/details/SafeQueue.hpp"
#include "FunctionTimeRegular.hpp"

using std::cout;
using std::endl;
using ::testing::Values;
using std::list;
using std::vector;
using std::thread;
using std::atomic;
using std::fixed;

int globalStaticIntX = 0;
struct BigThing{
    char thing[512];
};
SafeQueue<BigThing> sq;

void SignalTest::SetUp(){
}

void SignalTest::TearDown() {

}

void pushToQueue(BigThing bt){
    sq.enqueue(bt);
}

void staticSumFunction(int a, int b){
    globalStaticIntX = a+b;
    //cout << a << " + " << b << " = " << globalStaticIntX << endl;
}

TEST_F(SignalTest, SynchronousSignal){
    cout << "Instantiating signal object" << endl;
    
    Signal<int, int> testSignal;
    
    BasicTimer bt;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(SignalConnectionScheme::SYNCHRONOUS, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;
    
    cout << "Emitting signal" << endl;
    
    bt.start();
    testSignal.emitSignal(1, 2);
    bt.stop();
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    
    ASSERT_EQ(globalStaticIntX, 3);
    ASSERT_EQ(0, id);
    
    testSignal.disconnectSlot(0);
}

TEST_F(SignalTest, AsychronousSignal){
    cout << "Instantiating signal object" << endl;
    
    Signal<int, int> testSignal;
    
    BasicTimer bt;
    BasicTimer bt2;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(SignalConnectionScheme::ASYNCHRONOUS, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;
    
    cout << "Emitting signal" << endl;
    
    bt.start();
    bt2.start();
    testSignal.emitSignal(1, 2);
    bt.stop();
    
    while (bt2.getElapsedSeconds() < 0.1){
        if (globalStaticIntX == 3){
            bt2.stop();
            break;
        }
    }
    ASSERT_LT(bt2.getElapsedSeconds(), 0.1);
    ASSERT_EQ(globalStaticIntX, 3);
    
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Time to emit and process: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Net time to process: " << bt2.getElapsedMilliseconds() - bt.getElapsedMilliseconds() << "ms" << endl;
    
    ASSERT_EQ(0, id);
    
    testSignal.disconnectSlot(0);

}

TEST_F(SignalTest, AsychronousEnqueueSignal){
    cout << "Instantiating signal object" << endl;
    
    Signal<int, int> testSignal;
    
    BasicTimer bt;
    BasicTimer bt2;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;
    
    cout << "Emitting signal" << endl;
    
    bt.start();
    bt2.start();
    testSignal.emitSignal(1, 2);
    bt.stop();
    
    while (bt2.getElapsedSeconds() < 0.1){
        if (globalStaticIntX == 3){
            bt2.stop();
            break;
        }
    }
    ASSERT_LT(bt2.getElapsedSeconds(), 0.1);
    ASSERT_EQ(globalStaticIntX, 3);
    
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Time to emit and process: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Net time to process: " << bt2.getElapsedMilliseconds() - bt.getElapsedMilliseconds() << "ms" << endl;
     
    ASSERT_EQ(0, id);
    
    testSignal.disconnectAllSlots();

}

TEST_F(SignalTest, MultipleConnectionTypes){
    cout << "Testing multiple connection types for single signal" << endl;
    Signal<BigThing> testSignal;
    
    cout << "Connecting signals" << endl;
    list<int> ids;
    ids.push_back(testSignal.connectSlot(SignalConnectionScheme::SYNCHRONOUS, pushToQueue));
    ids.push_back(testSignal.connectSlot(SignalConnectionScheme::ASYNCHRONOUS, pushToQueue));
    ids.push_back(testSignal.connectSlot(SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE, pushToQueue));
    
    cout << "Emitting signal" << endl;
    BasicTimer bt;
    BasicTimer bt2;
    
    bt.start();
    bt2.start();
    BigThing bigthing;
    for (uint32_t i=0; i<10; i++){
        testSignal.emitSignal(bigthing);
    }
    bt2.stop();
    
    for (int i=0; i<30; i++){
        sq.dequeue();
    }
   
    bt.stop();
    
    cout << "Signals emitted" << endl;
    cout << "Total time for emission: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Total time for emission and processing: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emission+processing time: " << bt.getElapsedMilliseconds()/10.0 << "ms" << endl;
    ASSERT_LT(bt2.getElapsedSeconds(), 0.1);
    ASSERT_LT(bt.getElapsedSeconds(), 0.2);
}

class TestClass{
public:
    TestClass(){
        counter = 0;
        internalSignal.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::incrementCounter, this);
    }
    
    ~TestClass(){
    }
    
    void dostuff(int a) {
        cout << "Incrementing by " << a << endl;
        counter++;
        cout << "Result: " << counter << endl;
    }
    
    void dostuffconst(int a) const {
        cout << "Incrementing by " << a << endl;
        counter++;
        cout << "Result: " << counter << endl;
    }
    
    void emitIncrement(){
        cout << "Emitting increment signal from within class" << endl;
        internalSignal.emitSignal(1);
        cout << "Result: " << counter << endl;
    }
    
    void incrementCounter(uint32_t amount){
        counter += amount;
    }
    
    uint32_t getCounter(){
        return counter;
    }
    
    uint32_t getCounter() const{
        return counter;
    }
    
    mutable atomic<uint32_t> counter;
    Signal<uint32_t> internalSignal;
};

TEST_F(SignalTest, MemberFunction){
    TestClass tc;
    Signal<int> testSignal;
    
    testSignal.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuff, tc);
    testSignal.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc);
    
    testSignal.emitSignal(1);
    
    ASSERT_EQ(tc.getCounter(), 2u);
    
    tc.emitIncrement();
    
    ASSERT_EQ(tc.getCounter(), 3u);
    
    testSignal.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuff, &tc);
    
    testSignal.emitSignal(1);
    ASSERT_EQ(tc.getCounter(), 6u);
    
    const Signal<int> testSignal2;
    testSignal2.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc);
    
    testSignal2.emitSignal(1);
    
    ASSERT_EQ(tc.getCounter(), 7u); 
    
    const TestClass tc2;
    
    testSignal2.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc2);
    testSignal2.connectMemberSlot(SignalConnectionScheme::SYNCHRONOUS, &TestClass::dostuffconst, &tc2);
    testSignal2.emitSignal(1);
    ASSERT_EQ(tc2.getCounter(), 2u);
}

TEST_P(SignalTestParametrized, IntenseUsage){
    SignalTestParameters params = GetParam();
    BasicTimer bt, bt2;
    
    cout << "Intense usage test for signal type: ";
    switch(params.scheme){
        case(SignalConnectionScheme::ASYNCHRONOUS):
            cout << "Asynchronous";
            break;
        case(SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE):
        cout << "Asynchronous Enqueue";
        break;
        case(SignalConnectionScheme::SYNCHRONOUS):
        cout << "Synchronous";
        break;
        case (SignalConnectionScheme::THREAD_POOLED):
        cout << "Thread Pooled";
        break;
    }
    cout << endl;
    uint32_t counter = 0;
    atomic<uint32_t> completedFunctions;
    completedFunctions = 0;
    cout << "Emissions: " << params.nEmissions << ", Connections: " << params.nConnections <<
        ", Element Size: " << params.elementSize << ", Operations: " << params.nOperations << endl;
    
    typedef uint32_t sigType;
    auto func = ([&params, &completedFunctions](sigType x){
//        vector<char> res(vec.size());
//        for (uint32_t i=0; i<params.nOperations; i++){
//            memcpy(res.data(), vec.data(), vec.size());
//        }
        sigType  t;
        memcpy(&t, &x, sizeof(t));
        completedFunctions++;
    });
    
    Signal<sigType> signal;
    cout << "Connecting " << params.nConnections << " functions" << endl;
    for (uint32_t i=0; i<params.nConnections; i++){
        signal.connectSlot(params.scheme, func);
    }
    
    
        
    FunctionTimeRegular<> printer([this, &counter, &completedFunctions, &bt, &bt2, &params](){
        cout << "Total elements emitted: " << counter << " / " << params.nEmissions << fixed << endl;
        cout << "Emission rate: " << (double)counter/bt.getElapsedSeconds() <<  fixed << "m/s" << endl;
        cout << "Total elements processed: " << completedFunctions << " / " << params.nEmissions*params.nConnections << fixed << endl;
        cout << "Operation rate: " << (double)completedFunctions*params.nOperations/bt2.getElapsedSeconds() << "m/s" << endl;
        return (bt2.getElapsedSeconds() < 60);
    }, std::chrono::milliseconds(500));
    
    FunctionTimeRegular<> checkFinished([this, &params, &completedFunctions](){
        return (completedFunctions != params.nEmissions*params.nConnections);
    }, std::chrono::milliseconds(1));
    
    cout << "Starting emission thread: " << endl;
    thread emitter([&params, &signal, &bt, &bt2, &counter](){
        //sigType emitval(params.elementSize);
        sigType emitval = params.elementSize;
        bt2.start();
        bt.start();
        for (uint32_t i=0; i<params.nEmissions; i++){
            signal.emitSignal(emitval);
            counter++;
        }
        bt.stop();
        //counter+=params.nEmissions;
    });
    cout << "Emission thread spawned" << endl;
    cout.precision(std::numeric_limits<double>::max_digits10);

    ASSERT_LE(bt2.getElapsedSeconds(), 60);
    emitter.join();
    cout << "Emission completed" << endl;
    checkFinished.join();
    bt2.stop();
    printer.stopAndJoin();
    cout << "Processing completed" << endl;
    cout << "Emission rate: " << (double)counter/bt.getElapsedSeconds() <<  fixed << "m/s" << endl;
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emit time: " << (double)bt.getElapsedMilliseconds()/params.nEmissions << "ms" << endl;
    cout << "Time to emit+process: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emit+process time: " << (double)bt2.getElapsedMilliseconds()/params.nEmissions << "ms" << endl;
}

INSTANTIATE_TEST_CASE_P(
    SignalTest_Benchmark_Synchronized,
    SignalTestParametrized,
    Values(
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 100, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 10000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 10, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{100, 10, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1000, 10, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1, 100, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{1, 1000, 4, 1, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 10, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 100, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 100000, 4, 10, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 10000, 4, 100, SignalConnectionScheme::SYNCHRONOUS},                
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 10000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 10, 512, 1000, SignalConnectionScheme::SYNCHRONOUS},                
        SignalTestParameters{10, 1000, 512, 1000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 100000, 512, 1000, SignalConnectionScheme::SYNCHRONOUS},
        SignalTestParameters{10, 1000, 512, 50000, SignalConnectionScheme::SYNCHRONOUS}
    )
);
        
INSTANTIATE_TEST_CASE_P(
    SignalTest_Benchmark_Asynchronous,
    SignalTestParametrized,
    Values(
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 100, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 10000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{100, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1000, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1, 100, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{1, 1000, 4, 1, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 10, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 100, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 100000, 4, 10, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 10000, 4, 100, SignalConnectionScheme::ASYNCHRONOUS},                
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 1000, 4, 10000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 10, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS},                
        SignalTestParameters{10, 1000, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 100000, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS},
        SignalTestParameters{10, 1000, 512, 50000, SignalConnectionScheme::ASYNCHRONOUS}
    )
);
    
            
INSTANTIATE_TEST_CASE_P(
    SignalTest_Benchmark_AsynchronousEnqueue,
    SignalTestParametrized,
    Values(
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1, 10, 4, 100, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1, 10, 4, 10000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{100, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1000, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1, 100, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{1, 1000, 4, 1, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 1000, 4, 10, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 1000, 4, 100, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 100000, 4, 10, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 10000, 4, 100, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},                
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 1000, 4, 10000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 10, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},                
        SignalTestParameters{10, 1000, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 100000, 512, 1000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE},
        SignalTestParameters{10, 1000, 512, 50000, SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE}
    )
);
    
INSTANTIATE_TEST_CASE_P(
    SignalTest_Benchmark_ThreadPooled,
    SignalTestParametrized,
    Values(
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1, 10, 4, 100, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1, 10, 4, 10000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 10, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{100, 10, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1000, 10, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1, 10, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1, 100, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{1, 1000, 4, 1, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 1000, 4, 10, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 1000, 4, 100, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 100000, 4, 10, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 10000, 4, 100, SignalConnectionScheme::THREAD_POOLED},                
        SignalTestParameters{10, 1000, 4, 1000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 1000, 4, 10000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 10, 512, 1000, SignalConnectionScheme::THREAD_POOLED},                
        SignalTestParameters{10, 1000, 512, 1000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 100000, 512, 1000, SignalConnectionScheme::THREAD_POOLED},
        SignalTestParameters{10, 1000, 512, 50000, SignalConnectionScheme::THREAD_POOLED}
    )
);
