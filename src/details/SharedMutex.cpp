#include "BSignals/details/SharedMutex.h"

using BSignals::details::SharedMutex;

void SharedMutex::lock() {
    m_waitingWriters.fetch_add(1, std::memory_order_release);
    bool writerOld = m_writer.load(std::memory_order_relaxed);
    if (writerOld || !m_writer.compare_exchange_strong(writerOld, true, std::memory_order_seq_cst)){
        std::unique_lock<std::mutex> lock(m_writeMutex);
        writerOld = false;
        while (!m_writer.compare_exchange_weak(writerOld, true, std::memory_order_seq_cst)){
            m_writeConditionVariable.wait(lock);
            writerOld = false;
        }
    }
    m_waitingWriters.fetch_sub(1, std::memory_order_release);
    waitForReaders();
}

void SharedMutex::waitForReaders() {
    if (!m_readers.load(std::memory_order_acquire)) return;
    std::unique_lock<std::mutex> lock(m_writeMutex);
    while (m_readers.load(std::memory_order_acquire)){
        m_secondWriteConditionVariable.wait(lock);
    }
}

void SharedMutex::lock_shared() {
    if (m_writer.load(std::memory_order_acquire)){
        std::unique_lock<std::mutex> lock(m_readMutex);
        while (m_writer.load(std::memory_order_acquire) || m_waitingWriters.load(std::memory_order_acquire)) {
            m_readConditionVariable.wait(lock);
        }
    }
    m_readers.fetch_add(1, std::memory_order_release);
}

void SharedMutex::unlock() {
    m_writer.store(false, std::memory_order_release);
    if (m_waitingWriters.load(std::memory_order_acquire)) m_writeConditionVariable.notify_one();
    else m_readConditionVariable.notify_all();
}

void SharedMutex::unlock_shared() {
    m_readers.fetch_sub(1, std::memory_order_release);
    if (m_waitingWriters.load(std::memory_order_acquire)) m_secondWriteConditionVariable.notify_one();
}
