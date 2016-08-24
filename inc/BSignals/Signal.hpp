/* 
 * File:   Signal.hpp
 * Author: Barath Kannan
 *
 * Created on 14 June 2016, 9:12 PM
 */

#ifndef BSIGNALS_SIGNAL_HPP
#define BSIGNALS_SIGNAL_HPP

#include "BSignals/ExecutorScheme.h"
#include "BSignals/details/SignalImpl.hpp"

namespace BSignals {

template <typename... Args>
class Signal {
public:
    Signal() = default;

    Signal(bool enforceThreadSafety)
    : signalImpl(enforceThreadSafety) {}

    ~Signal() {}

    template<typename F, typename C>
    int connectMemberSlot(ExecutorScheme scheme, F&& function, C&& instance) {
        return signalImpl.connectMemberSlot(scheme, std::forward<F>(function), std::forward<C>(instance));
    }

    int connectSlot(ExecutorScheme scheme, std::function<void(Args...)> slot) {
        return signalImpl.connectSlot(scheme, slot);
    }

    void disconnectSlot(int id) {
        signalImpl.disconnectSlot(id);
    }

    void disconnectAllSlots() {
        signalImpl.disconnectAllSlots();
    }

    void emitSignal(const Args& ... p) {
        signalImpl.emitSignal(p...);
    }

    void invokeDeferred() {
        signalImpl.invokeDeferred();
    }

    void operator()(const Args& ... p) {
        signalImpl(p...);
    }

private:
    BSignals::details::SignalImpl<Args...> signalImpl;
    Signal<Args...>(const Signal<Args...>& that) = delete;
    void operator=(const Signal<Args...>&) = delete;
};

} /* namespace BSignals */

#endif /* BSIGNALS_SIGNAL_HPP */