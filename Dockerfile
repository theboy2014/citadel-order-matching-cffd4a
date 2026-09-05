# --- build stage: compile the engine ---
FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY test ./test
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target ome_engine ome_test \
    && ctest --test-dir build --output-on-failure

# --- runtime stage: just the binary + the rdkafka runtime ---
FROM debian:bookworm-slim AS runtime
RUN apt-get update && apt-get install -y --no-install-recommends \
        librdkafka1 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/build/ome_engine ./ome_engine
EXPOSE 9001
CMD ["./ome_engine"]
