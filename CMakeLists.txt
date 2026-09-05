cmake_minimum_required(VERSION 3.16)
project(ome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

# Header-only core (domain, book, queue, engine) lives in include/.
include_directories(include)

# The full engine binary (needs librdkafka for the Kafka publisher).
add_executable(ome_engine
    src/main.cpp
    src/gateway.cpp
    src/redis_snapshot.cpp
    src/kafka_publisher.cpp)
target_link_libraries(ome_engine PRIVATE Threads::Threads rdkafka)

# The matching test exercises only the header-only core — no Kafka/Redis/sockets.
add_executable(ome_test test/test_matching.cpp)
target_link_libraries(ome_test PRIVATE Threads::Threads)
enable_testing()
add_test(NAME matching COMMAND ome_test)
