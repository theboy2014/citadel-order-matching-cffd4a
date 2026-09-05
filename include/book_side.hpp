#pragma once
#include <deque>
#include <functional>
#include <map>
#include "domain.hpp"

namespace ome {

// One side of the book: a sorted ladder of price levels. Each level is a FIFO
// queue of resting orders, so within a price the earliest arrival is first.
// Comparator is templated so the Buy side sorts best-bid-first (descending)
// and the Sell side sorts best-ask-first (ascending).
template <typename Compare>
class BookSide {
public:
    using Level = std::deque<Order>;

    // Rest an order at its price level, appending it last (time priority).
    void rest(const Order& o) {
        levels_[o.price].push_back(o);
    }

    bool empty() const { return levels_.empty(); }

    // The best price available on this side, or nullptr when empty.
    const Order* best() const {
        if (levels_.empty()) return nullptr;
        return &levels_.begin()->second.front();
    }

    // Mutable access to the front order at the best level (the one to fill).
    Order* best_mut() {
        if (levels_.empty()) return nullptr;
        return &levels_.begin()->second.front();
    }

    // Drop the front order at the best level once it is fully filled, and
    // erase the level entirely if it becomes empty.
    void pop_best() {
        auto it = levels_.begin();
        it->second.pop_front();
        if (it->second.empty()) levels_.erase(it);
    }

    // Walk every resting order best-first (for snapshots).
    void for_each(const std::function<void(const Order&)>& fn) const {
        for (const auto& [price, level] : levels_) {
            for (const auto& o : level) fn(o);
        }
    }

private:
    std::map<Price, Level, Compare> levels_;
};

// Bids: highest price is "best", so sort descending.
using BidSide = BookSide<std::greater<Price>>;
// Asks: lowest price is "best", so sort ascending.
using AskSide = BookSide<std::less<Price>>;

} // namespace ome
