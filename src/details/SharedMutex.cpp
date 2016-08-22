#include "BSignals/details/SharedMutex.h"

using BSignals::details::SharedMutex;

void SharedMutex::lock() {
    std::unique_lock<std::mutex> lock(m_mutex);
    ++m_waitingWriters;
    while (m_readers != 0) {
        m_cv.wait(lock);
    }
    --m_waitingWriters;
    m_writer = true;
}

void SharedMutex::lock_shared() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_writer || m_waitingWriters) {
        m_cv.wait(lock);
    }
    ++m_readers;
}

void SharedMutex::unlock() {
    m_mutex.lock();
    m_writer = false;
    m_mutex.unlock();
    m_cv.notify_one();
}

void SharedMutex::unlock_shared() {
    m_mutex.lock();
    --m_readers;
    m_mutex.unlock();
    m_cv.notify_one();
}
