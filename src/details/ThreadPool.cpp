#include "BSignals/details/ThreadPool.h"
using std::atomic;
using std::vector;
using std::list;

const uint16_t ThreadPool::maxPoolThreads = 1000;
const uint16_t ThreadPool::basePoolThreads = 8;
MPMCQueue<std::function<void()>> ThreadPool::threadPooledFunctions(100);
//SafeQueue<std::function<void()>> ThreadPool::threadPooledFunctions;
list<std::thread> ThreadPool::pool{};

ThreadPool::_init ThreadPool::_initializer;

ThreadPool::_init::_init() {
    for (uint16_t i=0; i<basePoolThreads; i++){
        pool.emplace_back(std::thread(&ThreadPool::queueListener));
    }
}

ThreadPool::_init::~_init() {
    threadPooledFunctions.stop();
    for (std::thread &t : pool){
        t.join();
    }
}

void ThreadPool::queueListener() {
    while (!threadPooledFunctions.isStopped()){
        auto func = threadPooledFunctions.dequeue();
        if (threadPooledFunctions.isStopped())
            break;
        func();
    }
}


