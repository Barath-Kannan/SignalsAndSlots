// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

// Implementation of Dmitry Vyukov's MPMC algorithm
// http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
/* 
 * File:   ContiguousMPMCQueue.hpp
 * Author: Barath Kannan
 * This is an adaptation of https://github.com/mstump/queues/blob/master/include/mpmc-bounded-queue.hpp
 * Created on 20 June 2016, 1:01 AM
 */

#ifndef BSIGNALS_CONTIGUOUSMPMCQUEUE_HPP
#define BSIGNALS_CONTIGUOUSMPMCQUEUE_HPP

#include <atomic>
#include <type_traits>

namespace BSignals{ namespace details{
template<typename T, size_t N>
class ContiguousMPMCQueue
{
public:

    ContiguousMPMCQueue(){
        // make sure it's a power of 2
        static_assert((N != 0) && ((N & (~N + 1)) == N), "size of MPMC queue must be power of 2");

        // populate the sequence initial values
        for (size_t i=0; i<N; ++i){
            _buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    bool enqueue(const T& data){
        size_t  head_seq = _head_seq.load(std::memory_order_relaxed);
        while(true){
            node_t*  node     = &_buffer[head_seq & (N-1)];
            size_t   node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif      = (intptr_t) node_seq - (intptr_t) head_seq;

            if (dif == 0){
                if (_head_seq.compare_exchange_weak(head_seq, head_seq + 1, std::memory_order_relaxed)) {
                    node->data = data;
                    node->seq.store(head_seq + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                return false;
            }
            else{
                head_seq = _head_seq.load(std::memory_order_relaxed);
            }
        }
    }

    bool dequeue(T& data){
        size_t       tail_seq = _tail_seq.load(std::memory_order_relaxed);
        while(true){
            node_t*  node     = &_buffer[tail_seq & (N-1)];
            size_t   node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif      = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);

            // if seq and head_seq are the same then it means this slot is empty
            if (dif == 0) {
                // claim our spot by moving head
                // if head isn't the same as we last checked then that means someone beat us to the punch
                // weak compare is faster, but can return spurious results
                // which in this instance is OK, because it's in the loop
                if (_tail_seq.compare_exchange_weak(tail_seq, tail_seq + 1, std::memory_order_relaxed)) {
                    // set the output
                    data = node->data;
                    // set the sequence to what the head sequence should be next time around
                    node->seq.store(tail_seq + N, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                // if seq is less than head seq then it means this slot is full and therefore the buffer is full
                return false;
            }
            else{
                // under normal circumstances this branch should never be taken
                tail_seq = _tail_seq.load(std::memory_order_relaxed);
            }
        }
    }

private:

    struct node_t
    {
        T                     data;
        std::atomic<size_t>   seq;
    };

    typedef char cache_line_pad_t[64]; // it's either 32 or 64 so 64 is good enough

    cache_line_pad_t    _pad0;
    std::array<node_t, N> _buffer;
    cache_line_pad_t    _pad2;
    std::atomic<size_t> _head_seq{0};
    cache_line_pad_t    _pad3;
    std::atomic<size_t> _tail_seq{0};
    cache_line_pad_t    _pad4;

    ContiguousMPMCQueue(const ContiguousMPMCQueue&) {}
    void operator=(const ContiguousMPMCQueue&) {}
};
}}
#endif