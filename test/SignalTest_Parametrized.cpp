#include "SignalTest_Parametrized.h"
#include <iostream>
#include <list>
#include <vector>
#include <thread>
#include <atomic>

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

void SignalTestParametrized::SetUp() {
    auto tupleParams = GetParam();
    params = {::testing::get<0>(tupleParams), ::testing::get<1>(tupleParams), ::testing::get<2>(tupleParams), ::testing::get<3>(tupleParams), ::testing::get<4>(tupleParams), ::testing::get<5>(tupleParams)};
    
    cout << "Test for signal type: ";
    switch (params.scheme) {
        case (ExecutorScheme::DEFERRED_SYNCHRONOUS):
            return;
        case(ExecutorScheme::ASYNCHRONOUS):
            cout << "Asynchronous";
            break;
        case(ExecutorScheme::STRAND):
            cout << "Strand";
            break;
        case(ExecutorScheme::SYNCHRONOUS):
            cout << "Synchronous";
            break;
        case (ExecutorScheme::THREAD_POOLED):
            cout << "Thread Pooled";
            break;
    }
    cout << endl;
        
    cout << "Emissions: " << params.nEmissions << ", Connections: " << params.nConnections <<
            ", Operations: " << params.nOperations << ", Emitters: " << params.nEmitters <<
            ", Thread Safe: " << params.threadSafe << endl;
}

void SignalTestParametrized::TearDown() {
    cout << "Emission rate: " << (double) counter / bt.getElapsedMilliseconds() << fixed << "m/ms" << endl;
    cout << "Time to emit: " << bt.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emit time (overall): " << (double) bt.getElapsedNanoseconds() / (params.nEmissions) << "ns" << endl;
    cout << "Average emit time (per connection): " << (double) bt.getElapsedNanoseconds() / (params.nEmissions*params.nConnections) << "ns" << endl;
    cout << "Time to emit+process: " << bt2.getElapsedMilliseconds() << "ms" << endl;
    cout << "Average emit+process time (overall): " << (double) bt2.getElapsedNanoseconds() / params.nEmissions << "ns" << endl;
    cout << "Average emit+process time (per connection): " << (double) bt2.getElapsedNanoseconds() / (params.nEmissions*params.nConnections) << "ns" << endl;
}


TEST_P(SignalTestParametrized, IntenseUsage) {
    auto func = ([this](sigType x) {
        volatile sigType v = x;
        for (uint32_t i = 0; i < params.nOperations; i++) {
            v+=x;
        }
        completedFunctions++;
    });

    Signal<sigType> signal(params.threadSafe);
    cout << "Connecting " << params.nConnections << " functions" << endl;
    for (uint32_t i = 0; i < params.nConnections; i++) {
        signal.connectSlot(params.scheme, func);
    }

    FunctionTimeRegular<> printer([this]() {
        cout << "Total elements emitted: " << counter << " / " << params.nEmissions << fixed << endl;
        cout << "Emission rate: " << (double) counter / bt.getElapsedMilliseconds() << fixed << "m/ms" << endl;
        cout << "Total elements processed: " << completedFunctions << " / " << params.nEmissions * params.nConnections << fixed << endl;
        cout << "Operation rate: " << (double) completedFunctions * params.nOperations / bt2.getElapsedNanoseconds() << "m/ns" << endl;
        return (bt2.getElapsedSeconds() < 6000);
    }, std::chrono::milliseconds(500));

    FunctionTimeRegular<> checkFinished([this]() {
        return (completedFunctions != params.nEmissions * params.nConnections);
    }, std::chrono::milliseconds(1));

    cout << "Starting emission thread: " << endl;
    
    std::list<thread> emitters;
    for (uint32_t i=0; i<params.nEmitters; ++i){
        emitters.emplace_back([this, &signal, i]() {
            if (i==0){
                bt2.start();
                bt.start();
            }
            uint32_t current;
            while ((current = counter.fetch_add(1000)) < params.nEmissions-1000){
                for (uint32_t j=0; j<1000; ++j) signal.emitSignal(1);
            }
            if (current == params.nEmissions-1000){
                for (uint32_t j=0; j<1000; ++j) signal.emitSignal(1);
                bt.stop();
            }
        });
    }
    cout << "Emission thread spawned" << endl;
    cout.precision(std::numeric_limits<double>::max_digits10);
    for (auto &t : emitters){
        t.join();
    }
    cout << "Emission completed" << endl;
    checkFinished.join();
    bt2.stop();
    printer.stopAndJoin();
    cout << "Processing completed" << endl;
}


TEST_P(SignalTestParametrized, Correctness) {
    std::vector<uint32_t> doneFlags(params.nEmissions, 0);
    auto func = ([this, &doneFlags](sigType x) {
        volatile sigType v = x;
        for (uint32_t i = 0; i < params.nOperations; i++) {
            v+=x;
        }
        doneFlags[x] = 1;
        completedFunctions++;
    });

    Signal<sigType> signal(params.threadSafe);
    cout << "Connecting " << params.nConnections << " functions" << endl;
    for (uint32_t i = 0; i < params.nConnections; i++) {
        signal.connectSlot(params.scheme, func);
    }

    FunctionTimeRegular<> printer([this]() {
        cout << "Total elements emitted: " << counter << " / " << params.nEmissions << fixed << endl;
        cout << "Emission rate: " << (double) counter / bt.getElapsedMilliseconds() << fixed << "m/ms" << endl;
        cout << "Total elements processed: " << completedFunctions << " / " << params.nEmissions * params.nConnections << fixed << endl;
        cout << "Operation rate: " << (double) completedFunctions * params.nOperations / bt2.getElapsedNanoseconds() << "m/ns" << endl;
        return (bt2.getElapsedSeconds() < 6000);
    }, std::chrono::milliseconds(500));

    FunctionTimeRegular<> checkFinished([this]() {
        return (completedFunctions != params.nEmissions * params.nConnections);
    }, std::chrono::milliseconds(1));

    cout << "Starting emission thread: " << endl;
    
    std::list<thread> emitters;
    thread emitter([this, &signal]() {
        bt2.start();
        bt.start();
        for (counter=0; counter<params.nEmissions; ++counter) {
            signal.emitSignal(counter);
        }
        bt.stop();
    });
    cout << "Emission thread spawned" << endl;
    cout.precision(std::numeric_limits<double>::max_digits10);
    emitter.join();
    cout << "Emission completed" << endl;
    checkFinished.join();
    bt2.stop();
    printer.stopAndJoin();
    for (uint32_t i=0; i<params.nEmissions; i++) ASSERT_TRUE(doneFlags[i]);
}


TEST_P(SignalTestParametrized, LengthyUsage) {
    auto func = ([this](sigType x) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(params.nOperations*1000));
        completedFunctions.fetch_add(1);
    });

    Signal<sigType> signal(params.threadSafe);
    cout << "Connecting " << params.nConnections << " functions" << endl;
    for (uint32_t i = 0; i < params.nConnections; i++) {
        signal.connectSlot(params.scheme, func);
    }

    FunctionTimeRegular<> printer([this]() {
        cout << "Total elements emitted: " << counter << " / " << params.nEmissions << fixed << endl;
        cout << "Emission rate: " << (double) counter / bt.getElapsedMilliseconds() << fixed << "m/ms" << endl;
        cout << "Total elements processed: " << completedFunctions << " / " << params.nEmissions * params.nConnections << fixed << endl;
        cout << "Operation rate: " << (double) completedFunctions * params.nOperations / bt2.getElapsedNanoseconds() << "m/ns" << endl;
        return (bt2.getElapsedSeconds() < 6000);
    }, std::chrono::milliseconds(500));

    FunctionTimeRegular<> checkFinished([this]() {
        return (completedFunctions != params.nEmissions * params.nConnections);
    }, std::chrono::milliseconds(1));

    cout << "Starting emission thread: " << endl;
    
    thread emitter([this, &signal]() {
        bt2.start();
        bt.start();
        for (counter=0; counter<params.nEmissions; ++counter) {
            signal.emitSignal(1);
        }
        bt.stop();
    });
    cout << "Emission thread spawned" << endl;
    
    cout.precision(std::numeric_limits<double>::max_digits10);
    emitter.join();
    cout << "Emission completed" << endl;
    checkFinished.join();
    bt2.stop();
    printer.stopAndJoin();
}

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_Synchronous,
        SignalTestParametrized,
        testing::Combine(
        Values(1, 10, 50), //number of connections
        Values(10, 1000, 10000), //number of emissions per connection
        Values(1, 10, 100, 1000, 10000, 50000, 1000000), //number of operations in each function
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(false),
        Values(ExecutorScheme::SYNCHRONOUS)
        )
        );

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_Asynchronous,
        SignalTestParametrized,
        testing::Combine(
        Values(1, 10, 50), //number of connections
        Values(10, 1000, 10000), //number of emissions per connection
        Values(1, 10, 100, 1000, 10000, 50000, 1000000), //number of operations in each function
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(false),
        Values(ExecutorScheme::ASYNCHRONOUS)
        )
        );

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_Strand,
        SignalTestParametrized,
        testing::Combine(
        Values(1, 10, 50), //number of connections
        Values(10, 1000, 10000), //number of emissions per connection
        Values(1, 10, 100, 1000, 10000, 50000, 1000000), //number of operations in each function
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(false),
        Values(ExecutorScheme::STRAND)
        )
        );

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_ThreadPooled,
        SignalTestParametrized,
        testing::Combine(
        Values(1, 10, 50), //number of connections
        Values(10, 1000, 10000), //number of emissions per connection
        Values(1, 10, 100, 1000, 10000, 50000, 1000000), //number of operations in each function
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(false),
        Values(ExecutorScheme::THREAD_POOLED)
        )
        );

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_PureEmission,
        SignalTestParametrized,
        testing::Combine(
        Values(1), //nconnections
        Values(100000), //number of emissions
        Values(1), //number of operations
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(true, false),
        Values(ExecutorScheme::SYNCHRONOUS, ExecutorScheme::ASYNCHRONOUS, ExecutorScheme::STRAND, ExecutorScheme::THREAD_POOLED)
        )
        );

INSTANTIATE_TEST_CASE_P(
        SignalTest_Benchmark_PureEmissionPlus,
        SignalTestParametrized,
        testing::Combine(
        Values(1), //nconnections
        Values(100000000), //number of emissions
        Values(1), //number of operations
        Values(1, 2, 4, 8, 16, 32, 64), //number of emitters
        Values(true, false),
        Values(ExecutorScheme::SYNCHRONOUS, ExecutorScheme::STRAND, ExecutorScheme::THREAD_POOLED) //asynchronous is too slow for this
        )
        );