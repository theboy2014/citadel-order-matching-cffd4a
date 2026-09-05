#include "gateway.hpp"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace ome {

std::optional<Order> parse_order(const std::string& line) {
    std::istringstream in(line);
    std::string verb;
    long long price = 0, qty = 0;
    if (!(in >> verb >> price >> qty)) return std::nullopt;

    Side side;
    if (verb == "BUY") side = Side::Buy;
    else if (verb == "SELL") side = Side::Sell;
    else return std::nullopt;

    if (price <= 0 || qty <= 0) return std::nullopt;   // reject nonsense

    Order o{};
    o.side = side;
    o.price = static_cast<Price>(price);
    o.quantity = static_cast<Quantity>(qty);
    return o; // id and ts stamped later by the gateway
}

bool OrderGateway::admit(Order o) {
    const std::uint64_t n = seq_.fetch_add(1, std::memory_order_relaxed);
    o.id = n;
    o.ts = n;
    if (!inbound_.push(o)) {
        rejected_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    accepted_.fetch_add(1, std::memory_order_relaxed);
    return true;
}

void OrderGateway::start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    ::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    ::listen(listen_fd_, 16);

    running_.store(true, std::memory_order_release);
    std::thread([this] {
        while (running_.load(std::memory_order_acquire)) {
            int conn = ::accept(listen_fd_, nullptr, nullptr);
            if (conn < 0) continue;

            std::string buf;
            char chunk[256];
            ssize_t got;
            while ((got = ::read(conn, chunk, sizeof(chunk))) > 0) {
                buf.append(chunk, got);
                std::size_t nl;
                while ((nl = buf.find('\n')) != std::string::npos) {
                    std::string line = buf.substr(0, nl);
                    buf.erase(0, nl + 1);
                    auto parsed = parse_order(line);
                    if (parsed) admit(*parsed);
                    else rejected_.fetch_add(1, std::memory_order_relaxed);
                }
            }
            ::close(conn);
        }
    }).detach();
}

void OrderGateway::stop() {
    running_.store(false, std::memory_order_release);
    if (listen_fd_ >= 0) ::close(listen_fd_);
}

} // namespace ome
