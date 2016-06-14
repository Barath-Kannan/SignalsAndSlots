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
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <assert.h>

namespace BSignals{ namespace details{

template<typename T>
class MPSCQueue{
public:

    MPSCQueue() :
        _head(new buffer_node_t),
        _tail(_head.load(std::memory_order_relaxed)){
        buffer_node_t* front = _head.load(std::memory_order_relaxed);
        front->next.store(nullptr, std::memory_order_relaxed);
    }

    ~MPSCQueue(){
        T output;
        while (this->dequeue(output)) {}
        buffer_node_t* front = _head.load(std::memory_order_relaxed);
        delete front;
    }
    
    void enqueue(const T& input){
        buffer_node_t* node = (new buffer_node_t);
        node->data = input;
        node->next.store(nullptr, std::memory_order_relaxed);

        buffer_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);

        std::shared_lock<std::shared_timed_mutex> lock(_mutex);        
        if (waitingReader){
            _cv.notify_one();   
        }
    }

    bool dequeue(T& output){
        buffer_node_t* tail = _tail.load(std::memory_order_relaxed);
        buffer_node_t* next = tail->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return false;
        }

        output = next->data;
        _tail.store(next, std::memory_order_release);
        delete tail;
        return true;
    }
    
    void blockingDequeue(T& output){
        std::shared_lock<std::shared_timed_mutex> lock(_mutex);
        waitingReader = true;
        while (!dequeue(output)){
            _cv.wait(_mutex);
        }
        waitingReader = false;
    }
    
private:

    struct buffer_node_t{
        T                           data;
        std::atomic<buffer_node_t*> next;
    };

    std::atomic<buffer_node_t*> _head;
    std::atomic<buffer_node_t*> _tail;
    std::shared_timed_mutex _mutex;
    std::condition_variable_any _cv;
    bool waitingReader{false};
    
    MPSCQueue(const MPSCQueue&) {}
    void operator=(const MPSCQueue&) {}
};
}}

#endif
