#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include "engine.hpp"
#include "gateway.hpp"
#include "kafka_publisher.hpp"
#include "redis_snapshot.hpp"
#include "spsc_queue.hpp"

namespace {
std::atomic<bool> g_run{true};
void on_signal(int) { g_run.store(false, std::memory_order_release); }

std::string env_or(const char* name, const char* fallback) {
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string(fallback);
}
} // namespace

int main() {
    using namespace ome;
    const auto port = static_cast<std::uint16_t>(
        std::stoi(env_or("ORDER_PORT", "9001")));
    const std::string redis_host = env_or("REDIS_HOST", "127.0.0.1");
    const std::string kafka_brokers = env_or("KAFKA_BROKERS", "localhost:9092");

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    // 1) the lock-free seam between network and engine threads
    SpscQueue<Order> inbound(1 << 16);

    // 2) trades flow out to Kafka; the publisher IS the engine's sink
    KafkaPublisher publisher(kafka_brokers, "trades");
    MatchingEngine engine(inbound, [&publisher](const Trade& t) {
        publisher.publish(t);
    });

    // 3) the network edge feeds the same inbound queue
    OrderGateway gateway(port, inbound);

    // 4) the read-side mirror
    RedisSnapshotter redis(redis_host, 6379);
    redis.connect();

    engine.start();
    gateway.start();
    std::cout << "matching engine up on port " << port << "\n";

    // 5) snapshot top-of-book into Redis ~every 50ms until a signal
    while (g_run.load(std::memory_order_acquire)) {
        redis.snapshot("ome:book", engine.book());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "shutting down: " << gateway.accepted() << " accepted, "
              << gateway.rejected() << " rejected\n";
    gateway.stop();
    engine.stop();
    publisher.flush(5000);
    return 0;
}
