#include "BSignals/details/ThreadPool.h"
#include "BSignals/details/BasicTimer.h"
#include <iostream>
#include <algorithm>
using std::cout;
using std::endl;

using std::atomic;
using std::vector;
using std::list;

const uint16_t ThreadPool::maxPoolThreads = 8;
MPMCQueue<std::function<void()>> ThreadPool::threadPooledFunctions(1000000);
std::thread ThreadPool::poolMonitorThread;
atomic<uint32_t> ThreadPool::nThreads{0};
atomic<uint32_t> ThreadPool::realTasks{0};
std::function<void()> ThreadPool::bfc = nullptr;

ThreadPool::_init ThreadPool::_initializer;

ThreadPool::_init::_init() {
    poolMonitorThread = std::thread(&ThreadPool::poolMonitor);
}

ThreadPool::_init::~_init() {
    threadPooledFunctions.stop();
    poolMonitorThread.join();
}

void ThreadPool::queueListener() {
    while (!threadPooledFunctions.isStopped()){
        auto func = threadPooledFunctions.dequeue();
        if (threadPooledFunctions.isStopped() || !func)
            break;
        
        func();
        realTasks--;
    }
    nThreads--;
}

void ThreadPool::poolMonitor() {
    uint32_t lastTaskCount = 0;
    BasicTimer stopThreadTimer;
    std::chrono::duration<double> maxTimeBetweenEmits = std::chrono::seconds(0);
    std::chrono::high_resolution_clock::time_point epoch = std::chrono::high_resolution_clock::from_time_t(0);
    std::chrono::high_resolution_clock::time_point lastEmit = epoch;
    
    
    while (!threadPooledFunctions.isStopped()){
        
        //cout << "Pool size: " << nThreads << ", Real tasks: " << realTasks << endl;
        if (realTasks > nThreads && nThreads < maxPoolThreads){
            nThreads++;
            std::thread ql(&ThreadPool::queueListener);
            ql.detach();
        }
        else if (realTasks < nThreads && realTasks != 0){
            if (!stopThreadTimer.isRunning()){
                stopThreadTimer.start();
            }
            else if (stopThreadTimer.getElapsedDuration() > maxTimeBetweenEmits * 2){
                stopThreadTimer.stop();
                threadPooledFunctions.enqueue(bfc);
            }
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (realTasks > lastTaskCount) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            if (lastEmit == epoch) {
                lastEmit = currentTime;
            } else {
                maxTimeBetweenEmits = std::max<std::chrono::duration<double>>(currentTime - lastEmit, maxTimeBetweenEmits);
                lastEmit = currentTime;
            }
        }
        lastTaskCount = realTasks;
        //cout << "NThreads: " << nThreads << ", Tasks: " << realTasks << ", MTBE: " << maxTimeBetweenEmits.count() << endl;
        
    }
    
    for (uint32_t i=0; i<maxPoolThreads; i++){
        threadPooledFunctions.enqueue(bfc);
    }
    
    while (nThreads > 0){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
