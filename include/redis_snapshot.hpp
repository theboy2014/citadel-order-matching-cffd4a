#pragma once
#include <string>
#include "engine.hpp"

namespace ome {

// A minimal Redis client: connects over TCP and sends SET commands using the
// RESP protocol. Just enough to mirror top-of-book; not a general client.
class RedisSnapshotter {
public:
    RedisSnapshotter(std::string host, std::uint16_t port)
        : host_(std::move(host)), port_(port) {}

    bool connect();

    // Serialize best bid/ask + depth and SET it under `key`. Returns false on
    // any socket error so the caller can reconnect.
    bool snapshot(const std::string& key, const OrderBook& book);

private:
    bool send_set(const std::string& key, const std::string& value);

    std::string host_;
    std::uint16_t port_;
    int fd_{-1};
};

} // namespace ome
