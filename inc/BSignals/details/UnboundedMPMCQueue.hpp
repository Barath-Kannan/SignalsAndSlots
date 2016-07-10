/* 
 * File:   UnboundedMPMCQueue.hpp
 * Author: Barath Kannan
 *
 * Created on 9 July 2016, 1:57 AM
 */

#ifndef UNBOUNDEDMPMCQUEUE_HPP
#define UNBOUNDEDMPMCQUEUE_HPP

#include <atomic>
#include <shared_mutex>
#include <condition_variable>
#include <chrono>
#include <assert.h>
#include "ContiguousMPMCQueue.hpp"

namespace BSignals{ namespace details{

template<typename T>
class UnboundedMPMCQueue{
public:

    UnboundedMPMCQueue(){}

    ~UnboundedMPMCQueue(){
        T output;
        while (this->dequeue(output)) {}
        listNode* front = _head.load(std::memory_order_relaxed);
        delete front;
    }
    
    void enqueue(const T& input){
        if (_cache.enqueue(input)){
            std::shared_lock<std::shared_timed_mutex> lock(tailMutex);
            if (waitingReaders > 0) _cv.notify_one();
            return;
        }
        listNode* node = new listNode{input};
        listNode* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
        std::shared_lock<std::shared_timed_mutex> lock(tailMutex);        
        if (waitingReaders > 0) _cv.notify_one();
    }

    bool dequeue(T& output){
        if (_cache.dequeue(output)) return true;
        
        std::unique_lock<std::shared_timed_mutex> lock(tailMutex);
        listNode* tail = _tail;
        listNode* next = tail->next.load(std::memory_order_relaxed);
        if (next == nullptr) return false;

        output = next->data;
        _tail = next;
        lock.unlock();
        delete tail;
        return true;
    }
    
    void blockingDequeue(T& output){
        std::unique_lock<std::shared_timed_mutex> lock(tailMutex);
        waitingReaders.fetch_add(1);
        while (!unsafeDequeue(output)){
            _cv.wait(lock);
        }
        waitingReaders.fetch_sub(1);
    }

private:
    bool unsafeDequeue(T &output){
        if (_cache.dequeue(output)) return true;
        listNode* tail = _tail;
        listNode* next = tail->next.load(std::memory_order_relaxed);
        if (next == nullptr) return false;

        output = next->data;
        _tail = next;
        delete tail;
        return true;
    }
    
    struct listNode{
        T                           data;
        std::atomic<listNode*> next{nullptr};
    };
    
    std::atomic<listNode*> _head{new listNode};
    listNode* _tail{_head.load(std::memory_order_relaxed)};
    std::shared_timed_mutex tailMutex;
    std::condition_variable_any _cv;
    std::atomic<uint32_t> waitingReaders{0};
    
    ContiguousMPMCQueue<T, 256> _cache;
    
    
    UnboundedMPMCQueue(const UnboundedMPMCQueue&) {}
    void operator=(const UnboundedMPMCQueue&) {}
};
}}

#endif /* UNBOUNDEDMPMCQUEUE_HPP */

