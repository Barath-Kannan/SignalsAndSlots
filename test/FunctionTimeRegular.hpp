/* 
 * File:   FunctionTimeRegular.hpp
 * Author: Barath Kannan
 * Executes a function regularly on a thread
 * Created on May 6, 2016, 2:39 PM
 */

#ifndef BSIGNALS_FUNCTIONTIMEREGULAR_HPP
#define BSIGNALS_FUNCTIONTIMEREGULAR_HPP

#include <functional>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>

template <typename... Args>
class FunctionTimeRegular {
public:
    
    template<typename _Rep, typename _Period>
    FunctionTimeRegular(std::function<bool(Args... p)>&& function, std::chrono::duration<_Rep, _Period>&& interval){
        m_tfunc = function;
        m_interval = std::chrono::duration<double>(interval);
        mainThread = std::thread(&FunctionTimeRegular::threadLoop, this);
    }
    
    template<typename F, typename I, typename _Rep, typename _Period>
    FunctionTimeRegular(F&& function, I&& instance, std::chrono::duration<_Rep, _Period>&& interval){
        m_tfunc = std::function<bool(Args...)>(objectBind(function, instance));
        m_interval = std::chrono::duration<double>(interval);
        mainThread = std::thread(&FunctionTimeRegular::threadLoop, this);
    }
    
    ~FunctionTimeRegular(){
        stop();
        if (mainThread.joinable())
            mainThread.join();
    }
    
    void join(){
        mainThread.join();
    }
    
    void stop(){
        m_keepGoing=false;
        m_cv.notify_one();
    }
    
    void stopAndJoin(){
        stop();
        join();
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
    
    void threadLoop(){
        auto begin = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = std::chrono::high_resolution_clock::now() - begin;
        std::unique_lock<std::mutex> lock(m_mutex);
        for (std::chrono::duration<double> lastTime=elapsed; m_keepGoing; lastTime += m_interval){
            if (!m_tfunc())
                break;
            elapsed = std::chrono::high_resolution_clock::now() - begin;
            m_cv.wait_for(lock, m_interval - (elapsed-lastTime));
        }
    }
    
    mutable std::atomic<bool> m_keepGoing{true};
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
    mutable std::thread mainThread;
    mutable std::chrono::duration<double> m_interval;
    mutable std::function<bool(Args...)> m_tfunc;
};

#endif /* BSIGNALS_FUNCTIONTIMEREGULAR_HPP */

