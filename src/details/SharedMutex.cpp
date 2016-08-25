#include "BSignals/details/SharedMutex.h"
#include <thread>

using BSignals::details::SharedMutex;

void SharedMutex::lock() {
    m_writers.fetch_add(1, std::memory_order_release);
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_readers || m_writer){
        m_condVar.wait(lock);
    }
    m_writer.store(true, std::memory_order_release);
}

void SharedMutex::unlock() {
    m_writer.store(false, std::memory_order_release);
    m_writers.fetch_sub(1, std::memory_order_release);
    m_condVar.notify_one();
}

void SharedMutex::lock_shared(){
    m_readers.fetch_add(1, std::memory_order_release);
    if (m_writers.load(std::memory_order_acquire)){
        m_readers.fetch_sub(1, std::memory_order_release);
        while (m_writers.load(std::memory_order_acquire)){
            std::this_thread::yield();
        }
        m_readers.fetch_add(1, std::memory_order_release);
    }
}

void SharedMutex::unlock_shared() {
    m_readers.fetch_sub(1, std::memory_order_release);
    if (m_writers.load(std::memory_order_acquire)) m_condVar.notify_one();
}
