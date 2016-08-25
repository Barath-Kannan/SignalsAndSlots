#include "SignalTest.h"
#include <iostream>
#include <list>
#include <vector>
#include <thread>
#include <atomic>

#include "BSignals/details/BasicTimer.h"
#include "SafeQueue.hpp"
#include "FunctionTimeRegular.hpp"

using BSignals::details::SafeQueue;
using BSignals::details::BasicTimer;
using std::cout;
using std::endl;
using ::testing::Values;
using std::list;
using std::vector;
using std::thread;
using std::atomic;
using std::fixed;
using BSignals::Signal;
using BSignals::ExecutorScheme;

std::atomic<int> globalStaticIntX{0};

struct BigThing {
    char thing[512];
};
SafeQueue<BigThing> sq;

void SignalTest::SetUp() {
    globalStaticIntX = 0;
}

void SignalTest::TearDown() {

}

void pushToQueue(BigThing bt) {
    sq.enqueue(bt);
}

void staticSumFunction(int a, int b) {
    globalStaticIntX += a + b;
    //cout << a << " + " << b << " = " << globalStaticIntX << endl;
}

TEST_F(SignalTest, MetaTest){
    cout << "Instantiating signal object" << endl;

    Signal<int, int> testSignal;
    cout << "Size of signal: " << sizeof(testSignal) << endl;
    BSignals::details::MPSCQueue<std::function<void()>> mpscQ;
    cout << "Size of MPSCQ: " << sizeof(mpscQ) << endl;
    
    auto dur = BSignals::details::WheeledThreadPool::getMaxWait();
    cout << "Max wait: " << std::chrono::duration<double, std::nano>(dur).count() << endl;
}

TEST_F(SignalTest, SynchronousSignal) {
    cout << "Instantiating signal object" << endl;
    Signal<int, int> testSignal;
    BasicTimer bt;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(ExecutorScheme::SYNCHRONOUS, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;

    cout << "Emitting signal" << endl;

    bt.start();
    testSignal(1, 2);
    bt.stop();
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;

    ASSERT_EQ(globalStaticIntX, 3);
    ASSERT_EQ(0, id);

    testSignal.disconnectSlot(0);
}

TEST_F(SignalTest, DeferredSynchronousSignal) {
    cout << "Instantiating signal object" << endl;

    Signal<int, int> testSignal;

    BasicTimer bt;
    cout << "Connecting signal" << endl;
    bt.start();
    testSignal.connectSlot(ExecutorScheme::DEFERRED_SYNCHRONOUS, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;

    cout << "Emitting signal" << endl;

    bt.start();
    testSignal(1, 2);
    bt.stop();
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;

    ASSERT_NE(globalStaticIntX, 3);
    
    testSignal.invokeDeferred();
    
    ASSERT_EQ(globalStaticIntX, 3);

    testSignal.disconnectSlot(0);
}

TEST_F(SignalTest, AsynchronousSignal) {
    cout << "Instantiating signal object" << endl;

    Signal<int, int> testSignal;

    BasicTimer bt;
    BasicTimer bt2;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(ExecutorScheme::ASYNCHRONOUS, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;

    cout << "Emitting signal" << endl;

    bt.start();
    bt2.start();
    testSignal.emitSignal(1, 2);
    bt.stop();

    while (bt2.getElapsedSeconds() < 0.1) {
        if (globalStaticIntX == 3) {
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

TEST_F(SignalTest, StrandSignal) {
    cout << "Instantiating signal object" << endl;

    Signal<int, int> testSignal;

    BasicTimer bt;
    BasicTimer bt2;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(ExecutorScheme::STRAND, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;

    cout << "Emitting signal" << endl;

    bt.start();
    bt2.start();
    testSignal.emitSignal(1, 2);
    bt.stop();

    while (bt2.getElapsedSeconds() < 0.1) {
        if (globalStaticIntX == 3) {
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

TEST_F(SignalTest, MultipleConnectionTypes) {
    cout << "Testing multiple connection types for single signal" << endl;
    Signal<BigThing> testSignal;

    cout << "Connecting signals" << endl;
    list<int> ids;
    ids.push_back(testSignal.connectSlot(ExecutorScheme::SYNCHRONOUS, pushToQueue));
    ids.push_back(testSignal.connectSlot(ExecutorScheme::ASYNCHRONOUS, pushToQueue));
    ids.push_back(testSignal.connectSlot(ExecutorScheme::STRAND, pushToQueue));

    cout << "Emitting signal" << endl;
    BasicTimer bt;
    BasicTimer bt2;

    bt.start();
    bt2.start();
    BigThing bigthing;
    for (uint32_t i = 0; i < 10; i++) {
        testSignal.emitSignal(bigthing);
    }
    bt2.stop();

    cout << "Dequeueing" << endl;
    for (int i = 0; i < 30; i++) {
        sq.dequeue();
    }

    bt.stop();

    cout << "Signals emitted" << endl;
    cout << "Total time for emission: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Total time for emission and processing: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emission+processing time: " << bt.getElapsedMilliseconds() / 10.0 << "ms" << endl;
    ASSERT_LT(bt2.getElapsedSeconds(), 0.1);
    ASSERT_LT(bt.getElapsedSeconds(), 0.2);
}

class TestClass {
public:

    TestClass() {
        counter = 0;
        internalSignal.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::incrementCounter, this);
    }

    ~TestClass() {
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

    void emitIncrement() {
        cout << "Emitting increment signal from within class" << endl;
        internalSignal.emitSignal(1);
        cout << "Result: " << counter << endl;
    }

    void incrementCounter(uint32_t amount) {
        counter += amount;
    }

    uint32_t getCounter() {
        return counter;
    }

    uint32_t getCounter() const {
        return counter;
    }

    mutable atomic<uint32_t> counter;
    Signal<uint32_t> internalSignal;
};

TEST_F(SignalTest, MemberFunction) {
    TestClass tc;
    Signal<int> testSignal;

    testSignal.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuff, tc);
    testSignal.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc);

    testSignal.emitSignal(1);

    ASSERT_EQ(tc.getCounter(), 2u);

    tc.emitIncrement();

    ASSERT_EQ(tc.getCounter(), 3u);

    testSignal.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuff, &tc);

    testSignal.emitSignal(1);
    ASSERT_EQ(tc.getCounter(), 6u);

    Signal<int> testSignal2;
    testSignal2.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc);

    testSignal2.emitSignal(1);

    ASSERT_EQ(tc.getCounter(), 7u);

    const TestClass tc2;

    testSignal2.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuffconst, tc2);
    testSignal2.connectMemberSlot(ExecutorScheme::SYNCHRONOUS, &TestClass::dostuffconst, &tc2);
    testSignal2.emitSignal(1);
    ASSERT_EQ(tc2.getCounter(), 2u);
}

TEST_F(SignalTest, MultiEmit){
    cout << "Instantiating signal object" << endl;

    Signal<int, int> testSignal;

    BasicTimer bt;
    BasicTimer bt2;
    cout << "Connecting signal" << endl;
    bt.start();
    int id = testSignal.connectSlot(ExecutorScheme::STRAND, staticSumFunction);
    bt.stop();
    cout << "Time to connect: " << bt.getElapsedMilliseconds() << "ms" << endl;

    cout << "Emitting signal" << endl;

    bt.start();
    list<thread> threads;
    const uint32_t nThreads = 64;
    std::atomic<uint32_t> threadsRemaining{nThreads};
    for (uint32_t i=0; i<nThreads; i++){
        threads.emplace_back([&, i](){
            for (uint32_t i=0; i<1000000; ++i){
                testSignal.emitSignal(1, 2);
            }
            uint32_t tr = threadsRemaining.fetch_sub(1);
            cout << "Threads remaining: " << tr-1 << endl;
            cout << "globalStaticInt: " << globalStaticIntX << endl;
        });
    }
    bt.stop();
    
    bt2.start();
    while (bt2.getElapsedSeconds() < 60.0) {
        if (globalStaticIntX == 1000000*3*nThreads) {
            bt2.stop();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    //ASSERT_LT(bt2.getElapsedSeconds(), 0.1);
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Time to emit and process: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Net time to process: " << bt2.getElapsedMilliseconds() - bt.getElapsedMilliseconds() << "ms" << endl;

    ASSERT_EQ(0, id);

    testSignal.disconnectAllSlots();
    for (auto &t : threads) t.join();
}
