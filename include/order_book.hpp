#pragma once
#include <vector>
#include "book_side.hpp"
#include "domain.hpp"

namespace ome {

// The full two-sided book. Submitting an order matches it against the opposite
// side using price-time priority, then rests any unfilled remainder.
class OrderBook {
public:
    // Match `incoming` as far as it will go; append each fill to `out`.
    // Returns the unfilled remainder quantity (0 if fully filled).
    Quantity submit(Order incoming, std::vector<Trade>& out) {
        if (incoming.side == Side::Buy) {
            match(incoming, asks_, out);
            if (incoming.quantity > 0) bids_.rest(incoming);
        } else {
            match(incoming, bids_, out);
            if (incoming.quantity > 0) asks_.rest(incoming);
        }
        return incoming.quantity;
    }

    const BidSide& bids() const { return bids_; }
    const AskSide& asks() const { return asks_; }

private:
    // A buy crosses an ask when it offers at least the ask price; a sell
    // crosses a bid when it asks for at most the bid price.
    static bool crosses(Side incoming_side, Price incoming, Price resting) {
        return incoming_side == Side::Buy ? incoming >= resting
                                          : incoming <= resting;
    }

    template <typename OppositeSide>
    void match(Order& incoming, OppositeSide& opposite, std::vector<Trade>& out) {
        while (incoming.quantity > 0) {
            Order* top = opposite.best_mut();
            if (!top) break;                                   // book empty
            if (!crosses(incoming.side, incoming.price, top->price))
                break;                                         // no price overlap

            // Fill the smaller of the two remaining quantities, at the
            // resting order's price (price improvement goes to the taker).
            Quantity fill = std::min(incoming.quantity, top->quantity);
            const bool incoming_is_buy = incoming.side == Side::Buy;
            out.push_back(Trade{
                incoming_is_buy ? incoming.id : top->id,
                incoming_is_buy ? top->id : incoming.id,
                top->price,
                fill,
                incoming.ts,
            });

            incoming.quantity -= fill;
            top->quantity -= fill;
            if (top->quantity == 0) opposite.pop_best();       // resting order done
        }
    }

    BidSide bids_;
    AskSide asks_;
};

} // namespace ome
