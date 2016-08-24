#include "BSignals/details/SharedMutex.h"
#include <thread>

using BSignals::details::SharedMutex;

void SharedMutex::lock() {
    m_waitingWriters.fetch_add(1, std::memory_order_release);
    bool writerOld = false;
    while (!m_writer.compare_exchange_weak(writerOld, true, std::memory_order_seq_cst)){
        std::this_thread::yield();
    }
    m_waitingWriters.fetch_sub(1, std::memory_order_release);
    waitForReaders();
}

void SharedMutex::waitForReaders() {
    if (!m_readers.load(std::memory_order_acquire)) return;
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_readers.load(std::memory_order_acquire)){
        m_writeCV.wait(lock);
    }
}

void SharedMutex::lock_shared() {
    if (m_writer.load(std::memory_order_acquire)){
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_writer.load(std::memory_order_acquire) || m_waitingWriters.load(std::memory_order_acquire)) {
            m_readCV.wait(lock);
        }
    }
    m_readers.fetch_add(1, std::memory_order_release);
}

void SharedMutex::unlock() {
    m_writer.store(false, std::memory_order_release);
    if (!m_waitingWriters.load(std::memory_order_acquire)) m_readCV.notify_all();
}

void SharedMutex::unlock_shared() {
    m_readers.fetch_sub(1, std::memory_order_release);
    if (m_writer.load(std::memory_order_acquire)) m_writeCV.notify_one();
}
