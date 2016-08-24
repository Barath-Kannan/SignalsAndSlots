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
#include "BSignals/details/Slot.hpp"
#include "BSignals/details/StrandSlot.hpp"
#include "BSignals/details/ThreadPooledSlot.hpp"
#include "BSignals/details/AsynchronousSlot.hpp"
#include "BSignals/details/DeferredSlot.hpp"
#include "BSignals/details/SynchronousSlot.hpp"

namespace BSignals{ namespace details{

template <typename... Args>
class SignalImpl {
public:
    SignalImpl() = default;
    
    SignalImpl(bool enforceThreadSafety) 
        : enableEmissionGuard{enforceThreadSafety} {}
        
    ~SignalImpl(){
        disconnectAllSlots();
    }

    template<typename F, typename C>
    int connectMemberSlot(BSignals::ExecutorScheme scheme, F&& function, C&& instance){
        //type check assertions
        static_assert(std::is_member_function_pointer<F>::value, "function is not a member function");
        static_assert(std::is_object<std::remove_reference<C>>::value, "instance is not a class object");
        
        //Construct a bound function from the function pointer and object
        auto boundFunc = objectBind(function, instance);
        return connectSlot(scheme, boundFunc);
    }
    
    int connectSlot(BSignals::ExecutorScheme scheme, std::function<void(Args...)> slot){
        uint32_t id = currentId.fetch_add(1);
        if (enableEmissionGuard){
            std::lock_guard<std::mutex> lock(connectBufferLock);
            connectBuffer.emplace(id, ConnectDescriptor{scheme, slot});
            return id;
        }
        else{
            return connectSlotFunction(id, scheme, slot);
        }
    }
    
    void disconnectSlot(int id){
        if (enableEmissionGuard){
            slotLock.lock_shared();
            auto it = slots.find(id);
            if (it == slots.end()) return;
            it->second->markForDeath();
            slotLock.unlock_shared();
        }
        else{
            disconnectSlotFunction(id);
        }
    }
    
    void disconnectAllSlots(){
        slotLock.lock();
        slots.clear();
        slotLock.unlock();
    }
    
    void emitSignal(const Args& ... p){
        enableEmissionGuard ? emitSignalThreadSafe(p...) : emitSignalUnsafe(p...);
    }
    
    void invokeDeferred(){
        slotLock.lock_shared();
        std::pair<std::function<void()>, uint32_t> deferredInvocation;
        if (!deferredQueue) return;
        while (deferredQueue->dequeue(deferredInvocation)){
            if (!enableEmissionGuard || getIsStillConnectedFromExecutor(deferredInvocation.second)){
                deferredInvocation.first();
            }
        }
        slotLock.unlock_shared();
    }
    
    void operator()(const Args &... p){
        emitSignal(p...);
    }
    
private:
    SignalImpl<Args...>(const SignalImpl<Args...>& that) = delete;
    void operator=(const SignalImpl<Args...>&) = delete;
    
    inline void disconnectSlotFunction(uint32_t id){
        slotLock.lock();
        auto it = slots.find(id);
        if (it == slots.end()) return;
        slots.erase(it);
        slotLock.unlock();
    }
    
    int connectSlotFunction(uint32_t id, BSignals::ExecutorScheme scheme, std::function<void(Args...)> slot){
        slotLock.lock();
        switch(scheme){
            case(BSignals::ExecutorScheme::STRAND):
                slots.emplace(std::piecewise_construct, std::make_tuple(id),
                    std::make_tuple(new StrandSlot<Args...>(slot)));
                break;
            case(BSignals::ExecutorScheme::THREAD_POOLED):
                slots.emplace(std::piecewise_construct, std::make_tuple(id),
                    enableEmissionGuard ? std::make_tuple(new ThreadPooledSlot<Args...>(slot, [this, id](){return getIsStillConnectedFromExecutor(id);})) :
                        std::make_tuple(new ThreadPooledSlot<Args...>(slot)));
                break;
            case(BSignals::ExecutorScheme::ASYNCHRONOUS):
                slots.emplace(std::piecewise_construct, std::make_tuple(id),
                    enableEmissionGuard ? std::make_tuple(new AsynchronousSlot<Args...>(slot, [this, id](){return getIsStillConnectedFromExecutor(id);})) :
                        std::make_tuple(new AsynchronousSlot<Args...>(slot)));
                break;
            case(BSignals::ExecutorScheme::DEFERRED_SYNCHRONOUS):
                if (!deferredQueue) deferredQueue = std::shared_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>>(new MPSCQueue<std::pair<std::function<void()>, uint32_t>>);
                slots.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(new DeferredSlot<Args...>(slot, deferredQueue, id)));
                break;
            case(BSignals::ExecutorScheme::SYNCHRONOUS):
                slots.emplace(std::piecewise_construct, std::make_tuple(id),
                    std::make_tuple(new SynchronousSlot<Args...>(slot)));
                break;
        }
        slotLock.unlock();
        return (int)id;
    }
    
    inline void emitSignalUnsafe(const Args& ... p){
        for (auto const &kvpair : slots){
            kvpair.second->execute(p...);
        }
    }
    
    inline bool getIsStillConnectedFromExecutor(uint32_t id) const{
        slotLock.lock_shared();
        auto it = slots.find(id);
        bool retVal = (it != slots.end() && it->second->isAlive());
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
            slotLock.lock();
            for (auto it = slots.begin(); it!= slots.end();){
                if (!it->second->isAlive()){
                    it = slots.erase(it);
                }
                else{
                    ++it;
                }
            }
            slotLock.unlock();
        }
        slotLock.lock_shared();
        for (auto const &kvpair : slots){
            if (kvpair.second->isAlive()){
                kvpair.second->execute(p...);
            }
        }
        slotLock.unlock_shared();
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
    
    //Atomically incremented slotId
    std::atomic<uint32_t> currentId {0};
    
    //EmissionGuard determines if it is necessary to guard emission with a shared mutex
    //This is only required if connection/disconnection could be interleaved with emission
    const bool enableEmissionGuard {false};
    
    //Deferred invocation queue
    //Use unique pointer so that full MPSC queue overhead is only required if there is a deferred slot
    std::shared_ptr<MPSCQueue<std::pair<std::function<void()>, uint32_t>>> deferredQueue{nullptr};
    
    struct ConnectDescriptor{
        ExecutorScheme scheme;
        std::function<void(Args...)> slot;
    };
    
    mutable SharedMutex slotLock;
    std::map<uint32_t, std::unique_ptr<Slot<Args...>> > slots;
    
    mutable std::mutex connectBufferLock;
    std::map<uint32_t, ConnectDescriptor> connectBuffer;
};

}}

#endif /* BSIGNALS_SIGNALIMPL_HPP */
