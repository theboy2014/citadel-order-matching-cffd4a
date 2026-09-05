#include "kafka_publisher.hpp"
#include <librdkafka/rdkafka.h>
#include <sstream>
#include <string>

namespace ome {

KafkaPublisher::KafkaPublisher(std::string brokers, std::string topic)
    : brokers_(std::move(brokers)), topic_(std::move(topic)) {
    char err[256];
    rd_kafka_conf_t* conf = rd_kafka_conf_new();
    rd_kafka_conf_set(conf, "bootstrap.servers", brokers_.c_str(), err, sizeof(err));
    producer_ = rd_kafka_new(RD_KAFKA_PRODUCER, conf, err, sizeof(err));
}

KafkaPublisher::~KafkaPublisher() {
    if (producer_) {
        flush(5000);
        rd_kafka_destroy(static_cast<rd_kafka_t*>(producer_));
    }
}

void KafkaPublisher::publish(const Trade& t) {
    auto* producer = static_cast<rd_kafka_t*>(producer_);

    std::ostringstream body;
    body << "{\"buyId\":" << t.buy_id
         << ",\"sellId\":" << t.sell_id
         << ",\"price\":" << t.price
         << ",\"qty\":" << t.quantity
         << ",\"ts\":" << t.ts << "}";
    const std::string msg = body.str();
    const std::string key = std::to_string(t.price);

    rd_kafka_producev(
        producer,
        RD_KAFKA_V_TOPIC(topic_.c_str()),
        RD_KAFKA_V_KEY(key.data(), key.size()),
        RD_KAFKA_V_VALUE(const_cast<char*>(msg.data()), msg.size()),
        RD_KAFKA_V_END);
    rd_kafka_poll(producer, 0); // serve delivery callbacks, non-blocking
}

void KafkaPublisher::flush(int timeout_ms) {
    if (producer_) {
        rd_kafka_flush(static_cast<rd_kafka_t*>(producer_), timeout_ms);
    }
}

} // namespace ome
