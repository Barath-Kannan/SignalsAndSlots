#include "SafeQueueTest.h"
#include "BSignals/details/SafeQueue.hpp"
#include <thread>
#include <list>
#include <iostream>

#include "BSignals/details/BasicTimer.h"

using std::thread;
using std::list;
using std::cout;
using std::endl;
using std::pair;
using ::testing::Values;

void SafeQueueTest::SetUp(){}

void SafeQueueTest::TearDown(){}

TEST_F(SafeQueueTest, initialization){
    cout << "Running queue initialization test" << endl;
    SafeQueue<int> s;
    ASSERT_TRUE(s.isEmpty());
    ASSERT_TRUE(s.size() == 0);
}

TEST_F(SafeQueueTest, basicUsage){
    cout << "Running queue basic usage test" << endl;
    SafeQueue<int> s;
    
    s.enqueue(1);
    s.enqueue(2);
    s.enqueue(3);
    s.enqueue(4);
    
    ASSERT_FALSE(s.isEmpty());
    ASSERT_TRUE(s.size() == 4);
    
    for (int i=1; i<=4; i++){
        ASSERT_EQ(s.dequeue(), i);
        ASSERT_EQ((int)s.size(), 4 - i);
    }
    
    ASSERT_TRUE(s.isEmpty());
    ASSERT_TRUE(s.size() == 0);
}


TEST_P(SafeQueueTest_Usage_Parametrized, IntenseUsage){
    cout << "Running queue intense usage test" << endl;
    
    SafeQueue<int> s;
    
    int i=0;
    int lastElement = 0;
    BasicTimer t;
    t.start();
    
    int total = GetParam().first;
    int nworkers = GetParam().second;
    
    cout << "Total Items: " << total << ", Total Workers: " << nworkers << endl;
    
    thread mainReader([&s, &lastElement, &i, &t, total](){
        for (i=0; i<total; i++){
            lastElement = s.dequeue();
        }
        cout << "Master - Total time elapsed: " << t.getElapsedSeconds() << " seconds" << endl;
        ASSERT_LE(t.getElapsedSeconds(), 10.0);
    });
    
    list<thread> workers;
    for (int j=0; j<nworkers; j++){
        workers.push_back(thread([&s, &t, j, total, nworkers](){
            for (int k=0; k<total/nworkers; k++){
                s.enqueue(k);
            }
            if (j==nworkers-1){
                for (int k=0; k <total - j*(total/nworkers); k++){
                    s.enqueue(k);
                }
            }
        }));
    }
    
    thread checker([&t, &i, total](){
        while(i<total){
            double msgrate = ((double)i)/t.getElapsedSeconds() ;
            cout << "Checker thread: " << i <<
                    ", MsgRate: " << msgrate
                    << " msg/s, DataRate: " << msgrate * (sizeof(int)/1024.0) << "kb/s" << endl;
            ASSERT_LE(t.getElapsedSeconds(), 10.0);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
    
    mainReader.join();
    checker.join();
    for (auto &th : workers){
        th.join();
    }
}

INSTANTIATE_TEST_CASE_P(
    SafeQueue_HighFreqQueueUsage,
    SafeQueueTest_Usage_Parametrized,
    Values(
        pair<uint32_t, uint32_t>(1000, 4),
        pair<uint32_t, uint32_t>(10000, 4),
        pair<uint32_t, uint32_t>(100000, 4),
        pair<uint32_t, uint32_t>(1000000, 4),
        pair<uint32_t, uint32_t>(1000, 8),
        pair<uint32_t, uint32_t>(10000, 8),
        pair<uint32_t, uint32_t>(100000, 8),
        pair<uint32_t, uint32_t>(1000000, 8),
        pair<uint32_t, uint32_t>(5000000, 8),
        pair<uint32_t, uint32_t>(1000, 16),
        pair<uint32_t, uint32_t>(10000, 16),
        pair<uint32_t, uint32_t>(100000, 16),
        pair<uint32_t, uint32_t>(1000000, 16),
        pair<uint32_t, uint32_t>(5000000, 16),
        pair<uint32_t, uint32_t>(1000, 64),
        pair<uint32_t, uint32_t>(10000, 64),
        pair<uint32_t, uint32_t>(100000, 64),
        pair<uint32_t, uint32_t>(1000000, 64),
        pair<uint32_t, uint32_t>(5000000, 64),
        pair<uint32_t, uint32_t>(1000, 250),
        pair<uint32_t, uint32_t>(10000, 250),
        pair<uint32_t, uint32_t>(100000, 250),
        pair<uint32_t, uint32_t>(1000000, 250),
        pair<uint32_t, uint32_t>(5000000, 250)
    )
);

