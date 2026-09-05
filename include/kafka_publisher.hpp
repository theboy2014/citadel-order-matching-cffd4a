#pragma once
#include <string>
#include "domain.hpp"

namespace ome {

// Publishes executed Trade events to a Kafka topic. Wraps librdkafka; here we
// expose only what the engine needs: construct, publish one trade, flush.
class KafkaPublisher {
public:
    KafkaPublisher(std::string brokers, std::string topic);
    ~KafkaPublisher();

    // Publish one trade as a JSON message. The key is the price, so all fills
    // at a price land on the same partition and stay strictly ordered.
    void publish(const Trade& t);

    // Block until all in-flight messages are delivered (call on shutdown).
    void flush(int timeout_ms);

private:
    std::string brokers_;
    std::string topic_;
    void* producer_{nullptr}; // rd_kafka_t* (opaque to keep the header light)
};

} // namespace ome
