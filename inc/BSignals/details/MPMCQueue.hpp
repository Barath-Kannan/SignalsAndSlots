/* 
 * File:   MPMCQueue.hpp
 * Author: Barath Kannan
 *
 * Created on 10 July 2016, 3:20 PM
 */

#ifndef MPMCQUEUE_HPP
#define MPMCQUEUE_HPP

#include <atomic>
#include <shared_mutex>
#include "BSignals/details/ContiguousMPMCQueue.hpp"

namespace BSignals{ namespace details{

template<typename T>
class MPMCQueue{
public:
    MPMCQueue(){}

    ~MPMCQueue(){
        T output;
        while (this->dequeue(output)) {}
        listNode* front = _head.load(std::memory_order_relaxed);
        delete front;
    }
    
    void enqueue(const T& input){
        swapMutex.lock_shared();
        
        listNode *currentHead = _head.load(std::memory_order_relaxed);
        
        //succeeded in storing to head
        if (currentHead->_data.enqueue(input)){
            swapMutex.unlock_shared();
            return;
        }
        
        swapMutex.unlock_shared();
        
        //need a thread to resize
        swapMutex.lock();
        
        //if a different thread has already resized, we can return
        if (currentHead->_data.enqueue(input)){
            swapMutex.unlock();
            return;
        }
        
        //add a new contiguous queue to chain
        listNode* node = new listNode();
        node->_data.enqueue(input);
        listNode* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
        swapMutex.unlock();
    }
    
    bool dequeue(T& output){
        //try to dequeue from the contiguous queue at the tail
        swapMutex.lock_shared();
        listNode* tail = _tail.load(std::memory_order_relaxed);
        if (tail->_data.dequeue(output)){
            swapMutex.unlock_shared();
            return true;
        }
        
        if (tail == _head){
            swapMutex.unlock_shared();
            return false;
        }
        
        swapMutex.unlock_shared();
        
        //may need to throw away the last queue
        swapMutex.lock();
        tail = _tail.load(std::memory_order_relaxed);
        if (tail->_data.dequeue(output)){
            swapMutex.unlock();
            return true;
        }
        
        listNode* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr){
            swapMutex.unlock();
            return false;
        }

        _tail.store(next, std::memory_order_release);
        swapMutex.unlock();
        delete tail;
        swapMutex.lock_shared();
        bool ret = (next->_data.dequeue(output));
        swapMutex.unlock_shared();
        return ret;
    }
    
private:
    struct listNode{    
        ContiguousMPMCQueue<T, 8192> _data;
        std::atomic<listNode*> next{nullptr};
    };
    
    std::atomic<listNode*> _head{new listNode};
    std::atomic<listNode*> _tail{_head.load(std::memory_order_relaxed)};
    std::shared_timed_mutex swapMutex;
    std::condition_variable_any _cv;
    
    MPMCQueue(const MPMCQueue&) {}
    void operator=(const MPMCQueue&) {}

};

}}
#endif /* MPMCQUEUE_HPP */

