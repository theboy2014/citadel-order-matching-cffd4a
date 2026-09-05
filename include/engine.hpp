#pragma once
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include "order_book.hpp"
#include "spsc_queue.hpp"

namespace ome {

// Owns the order book and runs the single hot loop on its own thread. Orders
// arrive via the SPSC queue; executed trades are handed to a sink callback.
class MatchingEngine {
public:
    using TradeSink = std::function<void(const Trade&)>;

    MatchingEngine(SpscQueue<Order>& inbound, TradeSink sink)
        : inbound_(inbound), sink_(std::move(sink)) {}

    void start() {
        running_.store(true, std::memory_order_release);
        thread_ = std::thread([this] { run(); });
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        if (thread_.joinable()) thread_.join();
    }

    const OrderBook& book() const { return book_; }

private:
    void run() {
        Order order;
        std::vector<Trade> fills;
        while (running_.load(std::memory_order_acquire)) {
            if (!inbound_.pop(order)) {
                std::this_thread::yield(); // nothing to do; don't burn a core
                continue;
            }
            fills.clear();
            book_.submit(order, fills);
            for (const Trade& t : fills) sink_(t);
        }
    }

    SpscQueue<Order>& inbound_;
    TradeSink sink_;
    OrderBook book_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace ome
