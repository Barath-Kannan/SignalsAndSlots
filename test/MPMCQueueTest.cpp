#include "MPMCQueueTest.h"

#include <thread>
#include <list>
#include <iostream>
#include <algorithm>
#include "BSignals/details/BasicTimer.h"
#include "BSignals/details/MPMCQueue.hpp"
#include "BSignals/details/SafeQueue.hpp"

using BSignals::details::BasicTimer;
using BSignals::details::MPMCQueue;
using BSignals::details::SafeQueue;
using std::thread;
using std::list;
using std::cout;
using std::endl;
using std::fixed;
using std::pair;
using std::atomic;
using ::testing::Values;

void MPMCQueueTest::SetUp() {
}

void MPMCQueueTest::TearDown() {
}

TEST_F(MPMCQueueTest, ringtest) {

    struct bigThing {
        char x[256];
    };
    typedef bigThing ringtype;
    MPMCQueue<ringtype> ringbuf(100000);
    double elemSizeKb = sizeof (ringtype) / 1024.0;
    uint32_t totalReads = 0;
    uint32_t totalWrites = 0;
    
    uint32_t nThreadsWrite = 32;
    uint32_t nThreadsRead = 32;
    uint32_t nTotal = 80000000;
    uint32_t nValsWrite = nTotal / nThreadsWrite;
    uint32_t nValsRead = nTotal / nThreadsRead;    
    std::vector<uint32_t> partialWrites(100);
    std::vector<uint32_t> partialReads(100);
    for (uint32_t i=0; i<nThreadsWrite; i++) partialWrites[i] = 0;
    for (uint32_t i=0; i<nThreadsRead; i++) partialReads[i] = 0;
    
    BasicTimer bt;
    bt.start();
    list<thread> threads;
    for (uint32_t i = 0; i < nThreadsWrite; i++) {
        threads.emplace_back([&ringbuf, &totalWrites, &nValsWrite, i, &partialWrites]() {
            ringtype bt;
            cout << "Amount to write: " << nValsWrite << endl;
            for (uint32_t j = 0; j < nValsWrite; j++) {
                ringbuf.enqueue(bt);
                partialWrites[i] = partialWrites[i] +1;
            }
        });
    }
    for (uint32_t i = 0; i < nThreadsRead; i++) {
        threads.emplace_back([&ringbuf, &totalReads, &nValsRead, i, &partialReads]() {
            cout << "Amount to read: " << nValsRead << endl;
            for (uint32_t j = 0; j < nValsRead; j++) {
                ringbuf.dequeue();
                partialReads[i] = partialReads[i] +1;
            }
        });
    }
    cout.precision(std::numeric_limits<double>::max_digits10);
    BasicTimer bt2;
    bt2.start();
    while (bt.getElapsedSeconds() < 30) {
        if (bt2.getElapsedSeconds() > 0.5) {
            totalWrites = 0;
            totalReads = 0;
            for (uint32_t i=0; i<partialReads.size(); i++)
                totalReads += partialReads[i];
            for (uint32_t i : partialWrites)
                totalWrites += i;
            //cout << "RC: " << ringbuf.lastReadContention << ", WC: " << ringbuf.lastWriteContention << endl;
            cout << "TR: " << totalReads << ", TW: " << totalWrites << fixed << endl;
            double TRRate = (double) totalReads / bt.getElapsedSeconds();
            double TWRate = (double) totalWrites / bt.getElapsedSeconds();
            //cout << "Capacity: " << ringbuf.capacity() << endl;
            //cout << "RIndx: " << ringbuf.getReadIndex() << ", Windx: " << ringbuf.getWriteIndex() << endl;
            cout << "TRRate: " << TRRate << " msg/s" << fixed << endl;
            cout << "TRDataRate: " << TRRate * elemSizeKb << " kb/s" << fixed << endl;
            cout << "TWRate: " << TWRate << " msg/s" << fixed << endl;
            cout << "TWDataRate: " << TWRate * elemSizeKb << " kb/s" << fixed << endl;
            //cout << "Contention: " << ringbuf.getContention() << endl;
            cout << "Size: " << ringbuf.size() << endl;
            bt2.stop();
            bt2.start();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (totalReads == nTotal && totalWrites == nTotal)
            break;
    }
    bt.stop();
    ASSERT_LE(bt.getElapsedSeconds(), 30);
    cout << "Run time: " << bt.getElapsedSeconds() << "s" << endl;

    for (auto &t : threads) {
        t.join();
    }
}
