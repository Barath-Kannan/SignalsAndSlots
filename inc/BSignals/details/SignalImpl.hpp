/*
 * File:   SignalImpl.hpp
 * Author: Barath Kannan
 * Signal/Slots C++14 implementation
 * Created on May 10, 2016, 5:57 PM
 */

#ifndef BSIGNALS_SIGNALIMPL_HPP
#define BSIGNALS_SIGNALIMPL_HPP

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

#include "BSignals/ExecutorScheme.h"
#include "BSignals/details/MPSCQueue.hpp"
#include "BSignals/details/WheeledThreadPool.h"
#include "BSignals/details/Semaphore.h"

namespace BSignals{ namespace details{

template <typename... Args>
class SignalImpl {
public:
    SignalImpl() = default;
    
    SignalImpl(const bool& enforceThreadSafety) 
        : enableEmissionGuard{enforceThreadSafety} {}
        
    SignalImpl(const uint32_t& maxAsyncThreads) 
        : sem{maxAsyncThreads} {}
        
    SignalImpl(const bool& enforceThreadSafety, const uint32_t& maxAsyncThreads) 
        : enableEmissionGuard{enforceThreadSafety}, sem{maxAsyncThreads} {}
    
    ~SignalImpl(){
        disconnectAllSlots();
        sem.acquireAll();
    }

    template<typename F, typename C>
    int connectMemberSlot(const BSignals::ExecutorScheme& scheme, F&& function, C&& instance){
        //type check assertions
        static_assert(std::is_member_function_pointer<F>::value, "function is not a member function");
        static_assert(std::is_object<std::remove_reference<C>>::value, "instance is not a class object");
        
        //Construct a bound function from the function pointer and object
        auto boundFunc = objectBind(function, instance);
        return connectSlot(scheme, boundFunc);
    }
    
    int connectSlot(const BSignals::ExecutorScheme& scheme, std::function<void(Args...)> slot){
        uint32_t id = currentId.fetch_add(1);
        if (enableEmissionGuard){
            std::unique_lock<std::shared_timed_mutex> lock(backBufferLock);
            connectBuffer.emplace(id, ConnectDescriptor{scheme, slot});
            return id;
        }
        else{
            return connectSlotFunction(id, scheme, slot);
        }
    }
    
    void disconnectSlot(const int& id){
        if (enableEmissionGuard){
            std::unique_lock<std::shared_timed_mutex> lock(backBufferLock);
            disconnectBuffer.insert(id);
        }
        else{
            disconnectSlotFunction(id);
        }
    }
    
    void disconnectAllSlots(){
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        for (auto &q : strandQueues){
            q.second.enqueue(nullptr);
        }
        for (auto &t : strandThreads){
            t.second.join();
        }
        strandThreads.clear();
        strandQueues.clear();
        
        slots.clear();
    }
    
    void emitSignal(const Args& ... p){
        enableEmissionGuard ? emitSignalThreadSafe(p...) : emitSignalUnsafe(p...);
    }
    
    void invokeDeferred(){
        std::pair<std::function<void()>, uint32_t> deferredInvocation;
        while (deferredQueue.dequeue(deferredInvocation)){
            if (!enableEmissionGuard || getIsStillConnectedFromExecutor(deferredInvocation.second)){
                deferredInvocation.first();
            }
        }
    }
    
    void operator()(const Args &... p){
        emitSignal(p...);
    }
    
private:
    SignalImpl<Args...>(const SignalImpl<Args...>& that) = delete;
    void operator=(const SignalImpl<Args...>&) = delete;
    
    inline void disconnectSlotFunction(const int& id){
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        if (!slots.count(id)) return;
        auto &entry = slots[id];
        if (entry.scheme == BSignals::ExecutorScheme::STRAND){
            strandQueues[id].enqueue(nullptr);
            strandThreads[id].join();
            strandThreads.erase(id);
            strandQueues.erase(id);
        }
        slots.erase(id);
    }
    
    inline int connectSlotFunction(const int& id, const BSignals::ExecutorScheme& scheme, std::function<void(Args...)> slot){
        std::unique_lock<std::shared_timed_mutex> lock(signalLock);
        slots.emplace(id, ConnectDescriptor{scheme, slot});
        if (scheme == BSignals::ExecutorScheme::STRAND){
            strandQueues[id];
            strandThreads.emplace(id, std::thread(&SignalImpl::queueListener, this, id));
        }
        else if (scheme == BSignals::ExecutorScheme::THREAD_POOLED){
            WheeledThreadPool::startup();
        }
        return (int)id;
    }
    
    inline void invokeRelevantExecutor(const BSignals::ExecutorScheme& scheme, const uint32_t& id, const std::function<void(Args...)>& slot, const Args& ... p){
        switch(scheme){
            case(BSignals::ExecutorScheme::DEFERRED_SYNCHRONOUS):
                enqueueDeferred(id, slot, p...);
                return;
            case(BSignals::ExecutorScheme::SYNCHRONOUS):
                runSynchronous(id, slot, p...);
                return;
            case(BSignals::ExecutorScheme::ASYNCHRONOUS):
                runAsynchronous(id, slot, p...);
                return;
            case(BSignals::ExecutorScheme::STRAND):
                runStrands(id, slot, p...);
                return;
            case(BSignals::ExecutorScheme::THREAD_POOLED):
                runThreadPooled(id, slot, p...);
                return;
        }
    }
    
    inline void emitSignalUnsafe(const Args& ... p){
        for (auto const &kvpair : slots){
            invokeRelevantExecutor(kvpair.second.scheme, kvpair.first, kvpair.second.slot, p...);
        }
    }
    
    inline bool getIsStillConnected(const int& id) const{
        std::shared_lock<std::shared_timed_mutex> lock(backBufferLock);
        return (!disconnectBuffer.count(id));
    }
    
    inline bool getIsStillConnectedFromExecutor(const int& id) const{
        std::shared_lock<std::shared_timed_mutex> lock(backBufferLock);
        std::shared_lock<std::shared_timed_mutex> lockSignal(signalLock);
        return (!disconnectBuffer.count(id) && slots.count(id));
    }
    
    inline bool getHasWaitingDisconnects() const{
        std::shared_lock<std::shared_timed_mutex> lock(backBufferLock);
        return (!disconnectBuffer.empty());
    }
    
    inline bool getHasWaitingConnects() const{
        std::shared_lock<std::shared_timed_mutex> lock(backBufferLock);
        return (!connectBuffer.empty());
    }
    
    inline bool getHasWaitingConnectsOrDisconnects() const{
        std::shared_lock<std::shared_timed_mutex> lock(backBufferLock);
        return (!connectBuffer.empty() | !disconnectBuffer.empty());
    }
    
    inline void emitSignalThreadSafe(const Args& ... p){
        if (getHasWaitingConnectsOrDisconnects()){
            std::unique_lock<std::shared_timed_mutex> bbLock(backBufferLock);
            while (!connectBuffer.empty()){
                const std::pair<uint32_t, ConnectDescriptor> current = *connectBuffer.begin();
                connectBuffer.erase(connectBuffer.begin());
                //if there is another emit attempting to connect from inside a 
                //synchronous slot function it is going to attempt to push to the
                //backbuffer while holding a signal shared mutex. This will
                //cause a deadlock. Hence we need to release the backBufferLock
                //for the duration of the connect
                bbLock.unlock();
                connectSlotFunction(current.first, current.second.scheme, current.second.slot);
                bbLock.lock();
            }
            while (!disconnectBuffer.empty()){
                const uint32_t disconnectId = *disconnectBuffer.begin();
                disconnectBuffer.erase(disconnectBuffer.begin());
                //Same as above, need to relinquish the lock while we disconnect
                //to avoid deadlock
                bbLock.unlock();
                disconnectSlotFunction(disconnectId);
                bbLock.lock();
            }
        }
        std::shared_lock<std::shared_timed_mutex> lock(signalLock);
        for (auto const &kvpair : slots){
            if (getIsStillConnected(kvpair.first)){
                invokeRelevantExecutor(kvpair.second.scheme, kvpair.first, kvpair.second.slot, p...);
            }
        }
    }

    inline void runThreadPooled(const uint32_t& id, const std::function<void(Args...)> &function, const Args &... p) const {
        if (enableEmissionGuard){
            WheeledThreadPool::run([this, &id, &function, p...](){
                if (getIsStillConnectedFromExecutor(id)){
                    function(p...);
                }
            });
        }
        else{
            WheeledThreadPool::run([&function, p...](){
                function(p...);
            });
        }
    }
    
    inline void runAsynchronous(const uint32_t& id, const std::function<void(Args...)>& function, const Args& ... p){
        sem.acquire();
        std::thread slotThread([this, &id, &function, p...](){
            if (!enableEmissionGuard || getIsStillConnectedFromExecutor(id))
                function(p...);
            sem.release();
        });
        slotThread.detach();
    }
    
    inline void runStrands(const uint32_t& id, const std::function<void(Args...)>& function, const Args& ... p){
        //bind the function arguments to the function using a lambda and store
        //the newly bound function. This changes the function signature in the
        //resultant map, there are no longer any parameters in the bound function
        strandQueues[id].enqueue([&function, p...](){function(p...);});
    }
    
    inline void runSynchronous(const uint32_t& id, const std::function<void(Args...)>& function, const Args& ... p){
        function(p...);
    }
    
    inline void enqueueDeferred(const uint32_t& id, const std::function<void(Args...)>& function, const Args& ... p){
        deferredQueue.enqueue({[&function, p...](){function(p...);}, id});
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
        
    void queueListener(const uint32_t& id){
        auto &q = strandQueues[id];
        std::function<void()> func = [](){};
        auto maxWait = WheeledThreadPool::getMaxWait();
        std::chrono::duration<double> waitTime = std::chrono::nanoseconds(1);
        while (func){
            if (q.dequeue(func)){
                if (func) func();
                waitTime = std::chrono::nanoseconds(1);
            }
            else{
                std::this_thread::sleep_for(waitTime);
                    waitTime*=2;
            }
            if (waitTime > maxWait){
                q.blockingDequeue(func);
                if (func) func();
                waitTime = std::chrono::nanoseconds(1);
            }
        }
    }
    
    //Shared mutex for thread safety
    //Emit acquires shared lock, connect/disconnect acquires unique lock
    mutable std::shared_timed_mutex signalLock;
    
    //Async Emit Semaphore
    Semaphore sem {1024};
    
    //Atomically incremented slotId
    std::atomic<uint32_t> currentId {0};
    
    //EmissionGuard determines if it is necessary to guard emission with a shared mutex
    //This is only required if connection/disconnection could be interleaved with emission
    const bool enableEmissionGuard {false};
    
    //Strand Queues and Threads
    std::map<uint32_t, MPSCQueue<std::function<void()>>> strandQueues;
    std::map<uint32_t, std::thread> strandThreads;
    
    //Deferred invocation queue
    MPSCQueue<std::pair<std::function<void()>, uint32_t>> deferredQueue;
    
    struct ConnectDescriptor{
        ExecutorScheme scheme;
        std::function<void(Args...)> slot;
    };
    
    //Slot Map
    std::map<uint32_t, ConnectDescriptor> slots;
    
    //Disconnect backbuffer
    std::set<uint32_t> disconnectBuffer;
    
    //Connect backbuffer
    std::map<uint32_t, ConnectDescriptor> connectBuffer;
    
    //Backbuffer lock
    mutable std::shared_timed_mutex backBufferLock;
};

}}

#endif /* BSIGNALS_SIGNALIMPL_HPP */
