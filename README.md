# Citadel Securities | Low-Latency Order Matching Engine

An intermediate systems capstone. Model orders and trades with cache-friendly C++ types, build a price-time-priority limit order book, write the matching algorithm that crosses bids against asks, feed the engine over a single-producer/single-consumer lock-free ring buffer, run it on a dedicated thread, accept orders over a TCP gateway, snapshot top-of-book into Redis, publish executed trades to Kafka, and wire a small React dashboard — all packaged with Docker.

Built step-by-step with [KhwajaLabs Build](https://khwajalabs.com).

## Stack
- C++
- lock-free queues
- multithreading
- TCP sockets
- Redis
- Kafka
- Docker
