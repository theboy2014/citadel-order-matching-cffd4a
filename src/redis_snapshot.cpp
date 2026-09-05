#include "redis_snapshot.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace ome {

bool RedisSnapshotter::connect() {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    ::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr);
    return ::connect(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

// Build a RESP array: *3\r\n$3\r\nSET\r\n$<klen>\r\n<key>\r\n$<vlen>\r\n<val>\r\n
bool RedisSnapshotter::send_set(const std::string& key, const std::string& value) {
    std::ostringstream cmd;
    cmd << "*3\r\n"
        << "$3\r\nSET\r\n"
        << '$' << key.size() << "\r\n" << key << "\r\n"
        << '$' << value.size() << "\r\n" << value << "\r\n";
    const std::string out = cmd.str();
    return ::write(fd_, out.data(), out.size()) == static_cast<ssize_t>(out.size());
}

bool RedisSnapshotter::snapshot(const std::string& key, const OrderBook& book) {
    const Order* bid = book.bids().best();
    const Order* ask = book.asks().best();

    // {"bid":<price>,"bidQty":<qty>,"ask":<price>,"askQty":<qty>}
    std::ostringstream val;
    val << "{\"bid\":" << (bid ? bid->price : 0)
        << ",\"bidQty\":" << (bid ? bid->quantity : 0)
        << ",\"ask\":" << (ask ? ask->price : 0)
        << ",\"askQty\":" << (ask ? ask->quantity : 0) << "}";

    return send_set(key, val.str());
}

} // namespace ome
