#include "BSignals/details/WheeledThreadPool.h"
#include "BSignals/details/BasicTimer.h"
#include <algorithm>

using std::mutex;
using std::lock_guard;
using std::atomic;
using std::vector;
using BSignals::details::Wheel;
using BSignals::details::MPSCQueue;
using BSignals::details::SafeQueue;
using BSignals::details::WheeledThreadPool;
using BSignals::details::BasicTimer;

std::mutex WheeledThreadPool::tpLock;
bool WheeledThreadPool::isStarted = false;
std::chrono::duration<double> WheeledThreadPool::maxWait;
Wheel<MPSCQueue<std::function<void()>>, BSignals::details::WheeledThreadPool::nThreads> WheeledThreadPool::threadPooledFunctions {};
std::vector<std::thread> WheeledThreadPool::queueMonitors;

WheeledThreadPool::_init WheeledThreadPool::_initializer;

WheeledThreadPool::_init::_init(){
    //make a conservative estimate of when blocking will
    //be faster than spinning
    BasicTimer bt;
    MPSCQueue<int> tq;
    tq.enqueue(0);
    int x;
    bt.start();
    tq.blockingDequeue(x);
    bt.stop();
    maxWait = bt.getElapsedDuration()*2;
}

WheeledThreadPool::_init::~_init() {
    std::lock_guard<mutex> lock(tpLock);
    isStarted = false;
    for (uint32_t i=0; i<nThreads; i++){
        threadPooledFunctions.getSpoke(i).enqueue(nullptr);
    }
    for (auto &t : queueMonitors){
        t.join();
    }
}

void WheeledThreadPool::run(const std::function<void()> task) {
    threadPooledFunctions.getSpoke().enqueue(task);
}

void WheeledThreadPool::startup() {
    std::lock_guard<mutex> lock(tpLock);
    if (!isStarted){
        isStarted = true;
        for (unsigned int i=0; i<nThreads; ++i){
            queueMonitors.emplace_back(queueListener, i);
        }
    }
}

std::chrono::duration<double> WheeledThreadPool::getMaxWait() {
    return maxWait;
}

void WheeledThreadPool::queueListener(uint32_t index) {
    auto &spoke = threadPooledFunctions.getSpoke(index);
    std::function<void()> func;
    std::chrono::duration<double> waitTime = std::chrono::nanoseconds(1);
    while (isStarted){
        if (spoke.dequeue(func)){
            if (func) func();
            waitTime = std::chrono::nanoseconds(1);
        }
        else{
            std::this_thread::sleep_for(waitTime);
            waitTime*=2;
        }
        if (waitTime > maxWait){
            spoke.blockingDequeue(func);
            if (func) func();
            waitTime = std::chrono::nanoseconds(1);
        }
    }
}