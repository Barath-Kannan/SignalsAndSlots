// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

// C++ implementation of Dmitry Vyukov's non-intrusive lock free unbounded MPSC queue
// http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue
/* 
 * File:   MPSCQueue.hpp
 * Author: Barath Kannan
 * This is an adaptation of https://github.com/mstump/queues/blob/master/include/mpsc-queue.hpp
 * The queue has been modified such that it can also be used as a blocking queue
 * Aligned storage was removed, was causing segmentation faults and didn't improve performance
 * Created on 14 June 2016, 1:14 AM
 */


#ifndef MPSCQUEUE_HPP
#define MPSCQUEUE_HPP

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <assert.h>
#include "ContiguousMPMCQueue.hpp"
#include "ContiguousMPSCQueue.hpp"

namespace BSignals{ namespace details{

template<typename T>
class MPSCQueue{
public:

    MPSCQueue(){}

    ~MPSCQueue(){
        T output;
        while (this->dequeue(output)) {}
        listNode* front = _head.load(std::memory_order_relaxed);
        delete front;
    }
    
    void enqueue(const T& input){
        if (fastEnqueue(input)) return;
        slowEnqueue(input);
    }
    
    //only try to enqueue to the cache
    bool fastEnqueue(const T& input){
        if (_cache.enqueue(input)){    
            _cv.notify_one();
            return true;
        }
        return false;
    }

    bool dequeue(T& output){
        if (_cache.dequeue(output)) return true;
        return slowDequeue(output);
    }
    
    //only try to dequeue from the cache
    bool fastDequeue(T &output){
        if (_cache.dequeue(output)) return true;
        return false;
    }
    
    //transfer as many items as possible to the cache
    void transferMaxToCache(){
        T output;
        while (slowDequeue(output)){
            if (!fastEnqueue(output)){
                slowEnqueue(output);
                break;
            }
        }
    }
    
    void blockingDequeue(T& output){
        std::unique_lock<std::mutex> lock(_mutex);
        while (!dequeue(output)){
            _cv.wait(lock);
        }
    }
    
private:
    inline bool slowDequeue(T &output){
        listNode* tail = _tail.load(std::memory_order_relaxed);
        listNode* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr) return false;

        output = next->data;
        _tail.store(next, std::memory_order_release);
        delete tail;
        return true;
    }
    
    inline void slowEnqueue(const T& input){
        listNode* node = new listNode{input};

        listNode* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);

        _cv.notify_one();
    }
    
    struct listNode{
        T                           data;
        std::atomic<listNode*> next{nullptr};
    };
    ContiguousMPMCQueue<T, 256> _cache;
    
    std::atomic<listNode*> _head{new listNode};
    std::atomic<listNode*> _tail{_head.load(std::memory_order_relaxed)};
    std::mutex _mutex;
    std::condition_variable _cv;
    
    MPSCQueue(const MPSCQueue&) {}
    void operator=(const MPSCQueue&) {}
};
}}

#endif
