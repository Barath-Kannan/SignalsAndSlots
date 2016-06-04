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
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <type_traits>

#include "BSignals/details/SafeQueue.hpp"
#include "BSignals/details/ThreadPool.h"
#include "BSignals/details/Semaphore.h"

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
    THREAD_POOLED /*thread pooled mode in development*/
};

template <typename... Args>
class Signal {
public:
    //emissionGuard determines if it is necessary to guard emission with a shared mutex
    //this is only required if connection/disconnection could be interleaved with emission
    Signal(bool emissionGuard = false, uint16_t maxAsyncThreads = 100) : currentId(0), sem(maxAsyncThreads), enableEmissionGuard(emissionGuard)
    {}
    
    ~Signal(){
        disconnectAllSlots();
    }

    template<typename F, typename I>
    int connectMemberSlot(const SignalConnectionScheme &scheme, F&& function, I&& instance) const {
        //Construct a bound function from the function pointer and object
        auto boundFunc = objectBind(function, instance);
        return connectSlot(scheme, boundFunc);
    }
    
    int connectSlot(const SignalConnectionScheme &scheme, std::function<void(Args...)> slot) const {
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        uint32_t id = currentId.fetch_add(1);
        auto *slotMap = getSlotMap(scheme);
        slotMap->emplace(id, slot);
        if (scheme == SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE){
            asyncQueues[id];
            asyncQueueThreads.emplace(id, std::thread(&Signal::queueListener, this, id));
        }
        return (int)id;
    }
    
    void disconnectSlot(const uint32_t &id) const {
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        std::map<uint32_t, std::function<void(Args...)>> *slotMap = findSlotMapWithId(id);
        if (slotMap == nullptr)
            return;
        if (slotMap == &asynchronousEnqueueSlots){
            asyncQueues[id].stop();
            asyncQueueThreads[id].join();
            asyncQueueThreads.erase(id);
            asyncQueues.erase(id);
        }
        slotMap->erase(id);
    }
    
    void disconnectAllSlots() const { 
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        for (auto &q : asyncQueues){
            q.second.stop();
        }
        for (auto &t : asyncQueueThreads){
            t.second.join();
        }
        asyncQueueThreads.clear();
        asyncQueues.clear();
        
        synchronousSlots.clear();
        asynchronousSlots.clear();
        asynchronousEnqueueSlots.clear();
        threadPooledSlots.clear();
    }
    
    void emitSignal(const Args &... p) const {
        //wrapping the safe/thread-safe check like this seems to force the compiler
        //to perform the flag check statically...investigate if there is a way to
        //explicitly specify this
        
        return enableEmissionGuard ? emitSignalThreadSafe(p...) : emitSignalUnsafe(p...);
    }
    
private:
    void emitSignalUnsafe(const Args &... p) const {
        for (auto const &slot : synchronousSlots){
            runSynchronous(slot.second, p...);
        }
        
        for (auto const &slot : asynchronousSlots){
            runAsynchronous(slot.second, p...);
        }
        
        for (auto const &slot : asynchronousEnqueueSlots){
            runAsynchronousEnqueued(slot.first, slot.second, p...);
        }
        
        for (auto const &slot : threadPooledSlots){
            runThreadPooled(slot.second, p...);
        }
    }
    
    void emitSignalThreadSafe(const Args &... p) const {
        std::shared_lock<std::shared_timed_mutex> lock(signalLock);
        emitSignalUnsafe(p...);
    }

    void runThreadPooled(const std::function<void(Args...)> &function, Args... p) const {
        ThreadPool::run<Args...>(function, p...);
    }
    
    void runAsynchronous(const std::function<void(Args...)> &function, Args... p) const {
        sem.acquire();
        std::thread slotThread([this, &function, p...](){
            function(p...);
            sem.release();                
        });
        slotThread.detach();
    }
    
    void runAsynchronousEnqueued(uint32_t asyncQueueId, const std::function<void(Args...)> &function, Args... p) const{
        //bind the function arguments to the function using a lambda and store
        //the newly bound function. This changes the function signature in the
        //resultant map, there are no longer any parameters in the bound function
        asyncQueues[asyncQueueId].enqueue([&function, p...](){function(p...);});
    }
    
    void runSynchronous(const std::function<void(Args...)> &function, Args... p) const{
        function(p...);
    }
    
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
    
    std::map<uint32_t, std::function<void(Args...)>> *getSlotMap(const SignalConnectionScheme &scheme) const{
        switch(scheme){
            case (SignalConnectionScheme::ASYNCHRONOUS):
                return &asynchronousSlots;
            case (SignalConnectionScheme::ASYNCHRONOUS_ENQUEUE):
                return &asynchronousEnqueueSlots;
            case (SignalConnectionScheme::THREAD_POOLED):
                return &threadPooledSlots;
            default:
            case (SignalConnectionScheme::SYNCHRONOUS):
                return &synchronousSlots;
        }
    }
    
    std::map<uint32_t, std::function<void(Args...)>> *findSlotMapWithId(const uint32_t &id) const{
        if (synchronousSlots.count(id))
            return &synchronousSlots;
        if (asynchronousSlots.count(id))
            return &asynchronousSlots;
        if (asynchronousEnqueueSlots.count(id))
            return &asynchronousEnqueueSlots;
        if (threadPooledSlots.count(id))
            return &threadPooledSlots;
        return nullptr;
    }
    
    //shared mutex for thread safety
    //emit acquires shared lock, connect/disconnect acquires unique lock
    mutable std::shared_timed_mutex signalLock;
    
    //atomically incremented slotId
    mutable std::atomic<uint32_t> currentId;
    
    //Async Emit Semaphore
    mutable Semaphore sem;
        
    //Async Enqueue Queues and Threads
    mutable std::map<uint32_t, SafeQueue<std::function<void()>>> asyncQueues;
    mutable std::map<uint32_t, std::thread> asyncQueueThreads;
    
    //Slot Maps
    mutable std::map<uint32_t, std::function<void(Args...)>> synchronousSlots;
    mutable std::map<uint32_t, std::function<void(Args...)>> asynchronousSlots;
    mutable std::map<uint32_t, std::function<void(Args...)>> asynchronousEnqueueSlots;
    mutable std::map<uint32_t, std::function<void(Args...)>> threadPooledSlots;
    
    const bool enableEmissionGuard;
    
    void queueListener(const uint32_t &id) const{
        auto &q = asyncQueues[id];    
        while (!q.isStopped()){
            auto func = q.dequeue();
            if (q.isStopped())
                break;
            func();
        }
    }
};

#endif /* SIGNAL_HPP */
