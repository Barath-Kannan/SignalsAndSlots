
/* 
 * File:   Signal.hpp
 * Author: Barath Kannan
 *
 * Created on 14 June 2016, 9:12 PM
 */

#ifndef SIGNAL_HPP
#define SIGNAL_HPP

#include "BSignals/details/SignalImpl.hpp"

namespace BSignals{

enum class ExecutorScheme{
    SYNCHRONOUS,
    ASYNCHRONOUS, 
    STRAND,
    THREAD_POOLED
};
    
template <typename... Args>
class Signal{
public:
    Signal() = default;
    
    Signal(bool enforceThreadSafety) 
        : signalImpl(enforceThreadSafety){}
        
    Signal(uint32_t maxAsyncThreads) 
        : signalImpl(maxAsyncThreads) {}
        
    Signal(bool enforceThreadSafety, uint32_t maxAsyncThreads) 
        : signalImpl(enforceThreadSafety, maxAsyncThreads) {}
    
    ~Signal(){}

    template<typename F, typename C>
    int connectMemberSlot(const ExecutorScheme &scheme, F&& function, C&& instance) const {
        return signalImpl.connectMemberSlot((BSignals::details::ExecutorScheme)scheme, std::forward<F>(function), std::forward<C>(instance));
    }
    
    int connectSlot(const ExecutorScheme &scheme, std::function<void(Args...)> slot) const {
        return signalImpl.connectSlot((BSignals::details::ExecutorScheme)scheme, slot);
    }
    
    void disconnectSlot(const uint32_t &id) const {
        signalImpl.disconnectSlot(id);
    }
    
    void disconnectAllSlots() const { 
        signalImpl.disconnectAllSlots();
    }
    
    void emitSignal(const Args &... p) const {
        signalImpl.emitSignal(p...);
    }
    
private:
    BSignals::details::SignalImpl<Args...> signalImpl;
    Signal<Args...>(const Signal<Args...>& that) = delete;
    void operator=(const Signal<Args...>&) = delete;
};

} /* namespace BSignals */

#endif /* SIGNAL_HPP */

