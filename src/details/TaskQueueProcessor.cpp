#include "BSignals/details/TaskQueueProcessor.hpp"
#include "BSignals/details/BasicTimer.h"
#include <algorithm>
#include <iostream>

using std::cout;
using std::endl;

using std::mutex;
using std::lock_guard;
using std::atomic;
using std::vector;
using BSignals::details::UnboundedMPMCQueue;
using BSignals::details::BasicTimer;
using BSignals::details::TaskQueueProcessor;
using BSignals::details::MPSCQueue;

std::mutex TaskQueueProcessor::tpLock;
bool TaskQueueProcessor::isStarted = false;
std::chrono::duration<double> TaskQueueProcessor::maxWait;
BSignals::details::UnboundedMPMCQueue<std::function<void()> > TaskQueueProcessor::taskQueue;
//BSignals::details::MPMCQueue<std::function<void()> > TaskQueueProcessor::taskQueue;
//BSignals::details::MPSCQueue<std::function<void()> > TaskQueueProcessor::taskQueue;
std::vector<std::thread> TaskQueueProcessor::queueMonitors;

TaskQueueProcessor::_init TaskQueueProcessor::_initializer;

TaskQueueProcessor::_init::_init(){  
    //make a conservative estimate of when blocking will
    //be faster than spinning
    BasicTimer bt;
    UnboundedMPMCQueue<int> tq;
    tq.enqueue(0);
    int x;
    bt.start();
    //for (uint32_t i=0; i<1000; i++){
        tq.dequeue(x);
    //}
    bt.stop();
    maxWait = bt.getElapsedDuration()*2;
}

TaskQueueProcessor::_init::~_init() {
    std::lock_guard<mutex> lock(tpLock);
    isStarted = false;
    for (uint32_t i=0; i<nThreads; i++){
        taskQueue.enqueue(nullptr);
    }
    for (auto &t : queueMonitors){
        t.join();
    }
}

void TaskQueueProcessor::run(const std::function<void()> task) {
    taskQueue.enqueue(task);
}

void TaskQueueProcessor::startup() {
    std::lock_guard<mutex> lock(tpLock);
    if (!isStarted){
        isStarted = true;
        for (unsigned int i=0; i<nThreads; ++i){
            queueMonitors.emplace_back(queueListener, i);
        }
    }
}

void TaskQueueProcessor::queueListener(uint32_t index) {
    std::function<void()> func;
    std::chrono::duration<double> waitTime = std::chrono::nanoseconds(1);
    while (isStarted){
        if (func) func();
        if (taskQueue.dequeue(func)){
            if (func) func();
            waitTime = std::chrono::nanoseconds(1);
        }
        else if (waitTime < maxWait){
            std::this_thread::sleep_for(waitTime);
            waitTime*=2;
        }
        else{
//            taskQueue.blockingDequeue(func);
//            if (func) func();
//            waitTime = std::chrono::nanoseconds(1);
            std::this_thread::sleep_for(waitTime);
        }
    }
    
}