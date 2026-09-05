#pragma once
#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include "domain.hpp"
#include "spsc_queue.hpp"

namespace ome {

// Parses one line of the wire protocol into an Order (without id/ts, which the
// gateway stamps). Wire format: "BUY <price> <qty>" or "SELL <price> <qty>".
// Returns nullopt on any malformed input — the gateway rejects, never crashes.
std::optional<Order> parse_order(const std::string& line);

// Listens on a TCP port, reads newline-delimited orders, validates and stamps
// each, then pushes onto the inbound queue. Runs on its own (network) thread.
class OrderGateway {
public:
    OrderGateway(std::uint16_t port, SpscQueue<Order>& inbound)
        : port_(port), inbound_(inbound) {}

    void start();        // bind, listen, spawn the accept loop
    void stop();         // close the listener and join

    std::uint64_t accepted() const {
        return accepted_.load(std::memory_order_relaxed);
    }
    std::uint64_t rejected() const {
        return rejected_.load(std::memory_order_relaxed);
    }

private:
    // Stamp a monotonic id and timestamp, then enqueue. Returns false if the
    // queue is full (backpressure) — the caller rejects the order.
    bool admit(Order o);

    std::uint16_t port_;
    SpscQueue<Order>& inbound_;
    int listen_fd_{-1};
    std::atomic<bool> running_{false};
    std::atomic<std::uint64_t> seq_{0};       // next order id / timestamp
    std::atomic<std::uint64_t> accepted_{0};
    std::atomic<std::uint64_t> rejected_{0};
};

} // namespace ome
