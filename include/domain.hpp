#pragma once
#include <cstdint>
#include <string>

namespace ome {

// Buy or Sell. Scoped enum so a Side never silently converts to an int.
enum class Side : std::uint8_t { Buy, Sell };

// Monotonic id assigned by the gateway as orders arrive.
using OrderId = std::uint64_t;

// Prices are integer ticks (e.g. cents), never floating point. Equality and
// ordering must be exact, and floats are neither exact nor fast to compare.
using Price = std::int64_t;
using Quantity = std::uint64_t;

// A resting or incoming limit order. Trivially copyable POD: no heap, no
// virtuals, so it fits in a cache line and moves through queues for free.
struct Order {
    OrderId id;
    Side side;
    Price price;       // limit price in ticks
    Quantity quantity; // remaining shares
    std::uint64_t ts;  // arrival sequence, for time priority
};

// The immutable record of a fill between two orders at one price.
struct Trade {
    OrderId buy_id;
    OrderId sell_id;
    Price price;
    Quantity quantity;
    std::uint64_t ts;
};

inline const char* to_string(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

} // namespace ome
