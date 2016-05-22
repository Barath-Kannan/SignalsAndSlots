/*
 * File:   Signal.hpp
 * Author: Barath Kannan
 * Signal/Slots C++14 implementation
 * Created on May 10, 2016, 5:57 PM
 */

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include <functional>
#include <map>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <type_traits>

#include "BSignals/details/SafeQueue.hpp"
#include "BSignals/details/ThreadPool.h"

//These connection schemes determine how message emission to a connection occurs
    // SYNCHRONOUS CONNECTION:
    // Emission occurs synchronously.
    // When emit returns, all connected signals have returned.
    // This method is preferred when connected functions have short execution
    // time or when it is necessary to know that the function has returned 
    // before proceeding.

    // ASYNCHRONOUS CONNECTION:
    // Emission occurs asynchronously. A detached thread is spawned on emission.
    // When emit returns, the thread has been spawned. The thread automatically
    // destructs when the connected function returns.
    // This method is recommended when connected functions have long execution
    // time and are independent.

    // ASYNCHRONOUS ENQUEUED CONNECTION:
    // Emission occurs asynchronously. 
    // On connection a dedicated thread is spawned to wait for new messages.
    // Emitted parameters are bound to the mapped function and enqueued on the 
    // waiting thread. These messages are then processed synchronously in the
    // spawned thread.
    // This method is recommended when quick emission is required, connected
    // functions have longer execution time, and/or connected functions need to
    // be processed synchronously
//

enum class SignalConnectionScheme{
    SYNCHRONOUS,
    ASYNCHRONOUS, 
    ASYNCHRONOUS_ENQUEUE,
    THREAD_POOLED
};

template <typename... Args>
class Signal {
public:
    Signal(uint16_t maxAsyncThreads = 100) : currentId(0) {
        sem=maxAsyncThreads;
    }
    
    ~Signal(){
        disconnectAllSlots();
    }

    template<typename F, typename I>
    int connectMemberSlot(SignalConnectionScheme scheme, F&& function, I&& instance) const {
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        auto *slotMap = getSlotMap(scheme);       
        uint32_t id = currentId.fetch_add(1);
        //Construct a bound function from the function pointer and object
        //Store the bound function in the slotMap
        slotMap->insert({id, objectBind(function, instance)});
        if (slotMap == &asyncEnqueueSlots){
            asyncQueues[id];
            asyncQueueThreads.emplace(id, std::thread(&Signal::queueListener, this, id));
        }
        return (int)id;
    }
    
    int connectSlot(SignalConnectionScheme scheme, std::function<void(Args...)> slot) const{
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        auto *slotMap = getSlotMap(scheme);
        uint32_t id = currentId.fetch_add(1);
        slotMap->emplace(id, slot);
        if (slotMap == &asyncEnqueueSlots){
            asyncQueues[id];
            asyncQueueThreads.emplace(id, std::thread(&Signal::queueListener, this, id));
        }
        return (int)id;
    }
    
    void disconnectSlot(int id) const{
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        auto *slotMap = findSlotMapWithId(id);
        if (slotMap == nullptr)
            return;
        slotMap->erase(id);
        if (slotMap == &asyncEnqueueSlots){
            asyncQueues[id].stop();
            asyncQueueThreads[id].join();
            asyncQueueThreads.erase(id);
            asyncQueues.erase(id);
        }
    }
    
    void disconnectAllSlots() const { 
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        synchronousSlots.clear();
        asynchronousSlots.clear();
        
        for (auto &q : asyncQueues){
            q.second.stop();
        }
        
        for (auto &t : asyncQueueThreads){
            t.second.join();
        }
        asyncQueueThreads.clear();
        asyncQueues.clear();
        asyncEnqueueSlots.clear();

    }
    
    void emitSignal(Args... p) const{
        std::shared_lock<std::shared_timed_mutex> lock(signalLock);
        
        for (auto slot : threadPooledSlots){
            ThreadPool::run<Args...>(slot.second, p...);
        }
        
        //asynchronous enqueue slots are enqueued in the pre-instantiated thread
        //on their own dedicated thread
        for (auto slot : asyncEnqueueSlots){
            //bind the function arguments to the function and store the
            //newly bound function. This changes the function signature
            //in the resultant map, there are no longer any parameters 
            //in the bound function
            asyncQueues[slot.first].enqueue([=](){slot.second(p...);});
        }
        
        //run asynchronous slots on their own thread
        for (auto slot : asynchronousSlots){
            std::unique_lock<std::mutex> lock(semMutex);
            while (sem <= 0){
                semCondition.wait(lock);
            }
            sem--;
            lock.unlock();
            std::thread slotThread([this, slot, p...](){
                slot.second(p...);
                std::unique_lock<std::mutex> lock(semMutex);
                sem++;
                semCondition.notify_one();
            });
            slotThread.detach();
        }
        
        for (auto slot : synchronousSlots){
            slot.second(p...);
        }
    }

private:

    //Reference to instance
    template<typename F, typename I>
    std::function<void(Args...)> objectBind(F&& function, I&& instance) const {
        return[=, &instance](Args... args){
            (instance.*function)(args...);
        };
    }
    
    //Pointer to instance
    template<typename F, typename I>
    std::function<void(Args...)> objectBind(F&& function, I* instance) const {
        return objectBind(function, *instance);
    }
    
    std::map<uint32_t, std::function<void(Args...)>> *getSlotMap(SignalConnectionScheme scheme) const{
        switch(scheme){
            case (SignalConnectionScheme::ASYNCHRONOUS):
                return &asynchronousSlots;
            case (SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE):
                return &asyncEnqueueSlots;
            case (SignalConnectionScheme::THREAD_POOLED):
                return &threadPooledSlots;
            default:
            case (SignalConnectionScheme::SYNCHRONOUS):
                return &synchronousSlots;
        }
    }
    
    std::map<uint32_t, std::function<void(Args...)>> *findSlotMapWithId(int id) const{
        if (synchronousSlots.count(id))
            return &synchronousSlots;
        if (asynchronousSlots.count(id))
            return &asynchronousSlots;
        if (asyncEnqueueSlots.count(id))
            return &asyncEnqueueSlots;
        if (threadPooledSlots.count(id))
            return &threadPooledSlots;
        return nullptr;
    }
    
    mutable std::shared_timed_mutex signalLock;
    mutable std::atomic<uint32_t> currentId;
    
    //Synchronous
    mutable std::map<uint32_t, std::function<void(Args...)>> synchronousSlots;
    
    //Async Emit Thread
    mutable std::atomic<uint32_t> sem;
    mutable std::mutex semMutex;
    mutable std::condition_variable semCondition;
    mutable std::map<uint32_t, std::function<void(Args...)>> asynchronousSlots;
    
    //Async Enqueue 
    mutable std::map<uint32_t, std::function<void(Args...)>> asyncEnqueueSlots;
    mutable std::map<uint32_t, SafeQueue<std::function<void()>>> asyncQueues;
    mutable std::map<uint32_t, std::thread> asyncQueueThreads;
    
    void queueListener(uint32_t id) const{
        auto &q = asyncQueues[id];    
        while (!q.isStopped()){
            auto func = q.dequeue();
            if (q.isStopped())
                break;
            func();
        }
    }
    
    //Thread Pooled
    mutable std::map<uint32_t, std::function<void(Args...)>> threadPooledSlots;
    
};

#endif /* SIGNAL_HPP */

