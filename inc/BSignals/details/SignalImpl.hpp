/*
 * File:   SignalImpl.hpp
 * Author: Barath Kannan
 * Signal/Slots C++11 implementation
 * Created on May 10, 2016, 5:57 PM
 */

#ifndef BSIGNALS_SIGNALIMPL_HPP
#define BSIGNALS_SIGNALIMPL_HPP

#include <functional>
#include <map>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <type_traits>

#include "BSignals/ExecutorScheme.h"
#include "BSignals/details/MPSCQueue.hpp"
#include "BSignals/details/WheeledThreadPool.h"
#include "BSignals/details/Semaphore.h"
#include "BSignals/details/SharedMutex.h"

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
            std::lock_guard<std::mutex> lock(connectBufferLock);
            connectBuffer.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(scheme, slot));
            return id;
        }
        else{
            return connectSlotFunction(id, scheme, slot);
        }
    }
    
    void disconnectSlot(const int& id){
        if (enableEmissionGuard){
            slotLock.lock_shared();
            auto it = slots.find(id);
            if (it == slots.end()) return;
            it->second.alive.store(false, std::memory_order_release);
            slotLock.unlock_shared();
        }
        else{
            disconnectSlotFunction(id);
        }
    }
    
    void disconnectAllSlots(){
        slotLock.lock();
        for (auto &q : strandQueues){
            q.second.enqueue(nullptr);
        }
        for (auto &t : strandThreads){
            t.second.join();
        }
        strandThreads.clear();
        strandQueues.clear();
        
        slots.clear();
        slotLock.unlock();
    }
    
    void emitSignal(const Args& ... p){
        enableEmissionGuard ? emitSignalThreadSafe(p...) : emitSignalUnsafe(p...);
    }
    
    void invokeDeferred(){
        std::pair<std::function<void()>, uint32_t> deferredInvocation;
        if (!deferredQueue) return;
        while (deferredQueue->dequeue(deferredInvocation)){
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
        slotLock.lock();
        auto it = slots.find(id);
        if (it == slots.end()) return;
        if (it->second.scheme == BSignals::ExecutorScheme::STRAND){
            strandQueues[id].enqueue(nullptr);
            strandThreads[id].join();
            strandThreads.erase(id);
            strandQueues.erase(id);
        }
        slots.erase(it);
        slotLock.unlock();
    }
    
    inline int connectSlotFunction(const int& id, const BSignals::ExecutorScheme& scheme, std::function<void(Args...)> slot){
        slotLock.lock();
        slots.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(scheme, slot));
        if (scheme == BSignals::ExecutorScheme::STRAND){
            strandQueues[id];
            strandThreads.emplace(id, std::thread(&SignalImpl::queueListener, this, id));
        }
        else if (scheme == BSignals::ExecutorScheme::THREAD_POOLED){
            WheeledThreadPool::startup();
        }
        else if (scheme == BSignals::ExecutorScheme::DEFERRED_SYNCHRONOUS){
            if (!deferredQueue){
                deferredQueue = std::unique_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>>(new MPSCQueue<std::pair<std::function<void()>, uint32_t>>);
            }
        }
        slotLock.unlock();
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
    
    inline bool getIsStillConnectedFromExecutor(const int& id) const{
        slotLock.lock_shared();
        auto it = slots.find(id);
        bool retVal = ((it != slots.end()) && it->second.alive.load(std::memory_order_acquire));
        slotLock.unlock_shared();
        return retVal;
    }
    
    inline void emitSignalThreadSafe(const Args& ... p){
        if (!connectBuffer.empty()){
            connectBufferLock.lock();
            for (auto const &kvpair : connectBuffer){
                connectSlotFunction(kvpair.first, kvpair.second.scheme, kvpair.second.slot);
            }
            connectBuffer.clear();
            connectBufferLock.unlock();
            for (auto it = slots.begin(); it!= slots.end();){
                if (!it->second.alive.load(std::memory_order_acquire)){
                    it = slots.erase(it);
                }
                else{
                    ++it;
                }
            }
        }
        slotLock.lock_shared();
        for (auto const &kvpair : slots){
            if (kvpair.second.alive.load(std::memory_order_acquire)){
                invokeRelevantExecutor(kvpair.second.scheme, kvpair.first, kvpair.second.slot, p...);
            }
        }
        slotLock.unlock_shared();
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
        deferredQueue->enqueue({[&function, p...](){function(p...);}, id});
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
    //Use unique pointer so that full MPSC queue overhead is only required if there is a deferred slot
    std::unique_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>> deferredQueue{nullptr};
    
    class ConnectDescriptor{
    public:
        ConnectDescriptor(ExecutorScheme scheme_arg, std::function<void(Args...)> slot_arg) : scheme(scheme_arg), slot(slot_arg){}
        ExecutorScheme scheme;
        std::function<void(Args...)> slot;
        std::atomic<bool> alive{true};
    };
    
    mutable SharedMutex slotLock;
    std::map<uint32_t, ConnectDescriptor> slots;
    
    mutable std::mutex connectBufferLock;
    std::map<uint32_t, ConnectDescriptor> connectBuffer;
};

}}

#endif /* BSIGNALS_SIGNALIMPL_HPP */
