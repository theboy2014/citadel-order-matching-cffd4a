#pragma once
#include <atomic>
#include <cstddef>
#include <vector>

namespace ome {

// A bounded single-producer / single-consumer lock-free ring buffer.
// One thread only ever calls push(); one (different) thread only ever calls
// pop(). No mutex: progress is coordinated through two atomic indices.
template <typename T>
class SpscQueue {
public:
    explicit SpscQueue(std::size_t capacity)
        : buffer_(capacity), capacity_(capacity) {}

    // Producer side. Returns false if the queue is full (no overwrite).
    bool push(const T& item) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        const std::size_t next = increment(head);
        if (next == tail_.load(std::memory_order_acquire)) {
            return false; // full: consumer hasn't caught up
        }
        buffer_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer side. Returns false if the queue is empty.
    bool pop(T& out) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) {
            return false; // empty: nothing produced yet
        }
        out = buffer_[tail];
        tail_.store(increment(tail), std::memory_order_release);
        return true;
    }

private:
    std::size_t increment(std::size_t i) const {
        return (i + 1) % capacity_;
    }

    std::vector<T> buffer_;
    std::size_t capacity_;
    // Padded onto separate cache lines so the two threads don't false-share.
    alignas(64) std::atomic<std::size_t> head_{0}; // written by producer
    alignas(64) std::atomic<std::size_t> tail_{0}; // written by consumer
};

} // namespace ome
