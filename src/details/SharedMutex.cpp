#include "BSignals/details/SharedMutex.h"

using BSignals::details::SharedMutex;

void SharedMutex::lock() {
    if (m_waitingWriters.fetch_add(1, std::memory_order_release) == 0){
        m_writer.store(true, std::memory_order_release);
        m_waitingWriters.fetch_sub(1, std::memory_order_release);
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_readers.load(std::memory_order_acquire) != 0) {
        m_cv.wait(lock);
    }
    m_writer.store(true, std::memory_order_release);
    m_waitingWriters.fetch_sub(1, std::memory_order_release);
}

void SharedMutex::lock_shared() {
    if (!m_writer.load(std::memory_order_acquire) && !m_waitingWriters.load(std::memory_order_acquire)){
        m_readers.fetch_add(1, std::memory_order_release);
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_writer || m_waitingWriters) {
        m_cv.wait(lock);
    }
    m_readers.fetch_add(1, std::memory_order_release);
}

void SharedMutex::unlock() {
    m_writer.store(false, std::memory_order_release);
    m_cv.notify_one();
}

void SharedMutex::unlock_shared() {
    m_readers.fetch_sub(1, std::memory_order_release);
    if (m_waitingWriters.load(std::memory_order_acquire)) m_cv.notify_one();
}
