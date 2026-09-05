#include <cassert>
#include <iostream>
#include <vector>
#include "order_book.hpp"

using namespace ome;

// Helper: build an order with explicit fields (id == ts for time priority).
static Order mk(OrderId id, Side s, Price p, Quantity q) {
    return Order{id, s, p, q, id};
}

int main() {
    // 1) A resting ask, then a buy that crosses it fully.
    {
        OrderBook book;
        std::vector<Trade> fills;
        book.submit(mk(1, Side::Sell, 100, 10), fills); // rests, no fill
        assert(fills.empty());
        Quantity left = book.submit(mk(2, Side::Buy, 100, 10), fills);
        assert(left == 0);
        assert(fills.size() == 1);
        assert(fills[0].price == 100 && fills[0].quantity == 10);
        assert(fills[0].buy_id == 2 && fills[0].sell_id == 1);
    }

    // 2) Price-time priority: two asks at 100, the earlier one fills first.
    {
        OrderBook book;
        std::vector<Trade> fills;
        book.submit(mk(1, Side::Sell, 100, 5), fills);  // earlier
        book.submit(mk(2, Side::Sell, 100, 5), fills);  // later, same price
        fills.clear();
        book.submit(mk(3, Side::Buy, 100, 5), fills);   // takes 5
        assert(fills.size() == 1);
        assert(fills[0].sell_id == 1); // the earlier ask filled, not id 2
    }

    // 3) Price improvement: a buy at 105 trades at the resting ask of 100.
    {
        OrderBook book;
        std::vector<Trade> fills;
        book.submit(mk(1, Side::Sell, 100, 10), fills);
        fills.clear();
        book.submit(mk(2, Side::Buy, 105, 10), fills);
        assert(fills.size() == 1);
        assert(fills[0].price == 100); // executed at the resting price, not 105
    }

    // 4) Partial fill + sweep: a big buy eats two ask levels and rests the rest.
    {
        OrderBook book;
        std::vector<Trade> fills;
        book.submit(mk(1, Side::Sell, 100, 4), fills);
        book.submit(mk(2, Side::Sell, 101, 4), fills);
        fills.clear();
        Quantity left = book.submit(mk(3, Side::Buy, 101, 10), fills);
        assert(fills.size() == 2);          // swept both levels
        assert(left == 2);                  // 10 - 4 - 4 = 2 rests on the bid
        assert(book.bids().best() != nullptr);
        assert(book.bids().best()->price == 101);
    }

    std::cout << "all matching tests passed\n";
    return 0;
}
