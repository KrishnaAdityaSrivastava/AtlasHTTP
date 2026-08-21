# AtlasHTTP

**AtlasHTTP** is a Linux-focused HTTP/1.x server framework built from scratch in C++17.

It implements the HTTP request lifecycle directly on top of POSIX sockets, non-blocking I/O, Linux `epoll`, worker-thread scheduling, `writev()`, and `sendfile()` without third-party networking frameworks.

The project started as a systems-programming exercise and evolved into a performance-engineering project focused on understanding what happens beneath higher-level web frameworks: connection management, event-driven networking, HTTP parsing, routing, concurrency, response serialization, and static-file delivery.

---

## Highlights

* HTTP/1.0 and HTTP/1.1 support
* Linux `epoll` event-driven networking
* Non-blocking TCP sockets
* Reusable worker-thread pool
* Lambda-based GET and POST route handlers
* Dynamic route parameters such as `/users/:id`
* Incremental HTTP request parsing
* `writev()` scatter/gather response writes
* `sendfile()` for static-file delivery
* MIME type detection
* Path traversal protection
* Request header and body size limits
* CMake build system
* Automated tests with CTest
* Linux `perf` profiling and benchmark-driven optimization

---

## Performance at a Glance

AtlasHTTP was iteratively optimized by benchmarking each architectural change and profiling the resulting implementation.

| Stage       | Architecture            |        Throughput | Avg Latency | p99 Latency |
| ----------- | ----------------------- | ----------------: | ----------: | ----------: |
| Baseline    | Single-threaded         |      ~5,100 req/s |     9.87 ms |   154.00 ms |
| Thread Pool | Worker-thread execution |     ~21,489 req/s |    23.50 ms |    48.85 ms |
| Epoll       | Event-driven networking |     ~43,308 req/s |    18.40 ms |    21.92 ms |
| Current     | Dynamic route workload  | **~86,898 req/s** | **5.27 ms** |    28.23 ms |

The final implementation achieved more than **17× the throughput** of the original single-threaded version on the tested workload.

### NGINX Comparison

Dynamic route benchmark:

| Server           | Requests/sec | Avg Latency | Transfer/sec |
| ---------------- | -----------: | ----------: | -----------: |
| AtlasHTTP        |   **86,897** |     5.27 ms |   11.77 MB/s |
| NGINX (`return`) |       18,447 |     4.94 ms |    2.71 MB/s |

Static file benchmark:

| Server    | Requests/sec | Avg Latency | Transfer/sec |
| --------- | -----------: | ----------: | -----------: |
| AtlasHTTP |       12,850 |    38.15 ms |   11.72 MB/s |
| NGINX     |   **17,238** |     6.31 ms |   18.17 MB/s |

AtlasHTTP achieved approximately **75% of NGINX's throughput** on the static-file workload.

The dynamic benchmark is primarily useful for evaluating the custom framework's request-processing pipeline. The static benchmark provides a more representative comparison because both servers perform request parsing, routing, file handling, and response transmission.

> These benchmarks are architectural experiments rather than claims of production-server superiority.

---

# Installation

## Requirements

AtlasHTTP currently targets Linux because it relies on Linux/POSIX APIs such as `epoll`, `writev()`, and `sendfile()`.

You need:

* Linux
* C++17-compatible compiler
* CMake 3.16+
* pthreads

No third-party C++ networking libraries are required.

## Clone

```bash
git clone https://github.com/KrishnaAdityaSrivastava/AtlasHTTP.git
cd AtlasHTTP
```

## Build

Configure and build:

```bash
cmake -S . -B build
cmake --build build --parallel
```

For a Release build:

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel
```

Or use the included preset:

```bash
cmake --preset default
cmake --build --preset default
```

## Build Options

Examples and tests can be explicitly enabled with:

```bash
cmake -S . -B build \
    -DATLASHTTP_BUILD_EXAMPLES=ON \
    -DATLASHTTP_BUILD_TESTS=ON
```

## Run

Build and run the example server:

```bash
./build/atlas_http_server
```

The example server listens on:

```text
127.0.0.1:8080
```

Test it with:

```bash
curl http://127.0.0.1:8080/
```

## Test

Run the automated test suite:

```bash
ctest --test-dir build --output-on-failure
```

Current tests cover route matching, dynamic path parameters, and router dispatch.

---

# Quick Start

Include the public AtlasHTTP API:

```cpp
#include <AtlasHTTP/Server/server.hpp>

#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>

int main() {
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(
        AF_INET,
        SOCK_STREAM,
        0,
        8080,
        INADDR_ANY,
        16,
        4
    );

    server.get("/", [](const HTTP::Request&) {
        HTTP::Response res(
            200,
            "<h1>Hello, World!</h1>"
        );

        res.headers["Content-Type"] = "text/html";
        return res;
    });

    server.launch();
}
```

The constructor configures:

```cpp
HTTP::Server server(
    AF_INET,      // address family
    SOCK_STREAM,  // socket type
    0,            // protocol
    8080,         // port
    INADDR_ANY,   // interface
    16,            // listen backlog
    4             // worker threads
);
```

### Dynamic Routes

```cpp
server.get("/users/:id", [](const HTTP::Request& req) {
    return HTTP::Response(
        200,
        req.params.at("id")
    );
});
```

A request to:

```text
GET /users/42
```

makes `42` available through:

```cpp
req.params.at("id")
```

### Static Files

Static responses are resolved relative to `./public`:

```cpp
server.get("/files/:name", [](const HTTP::Request& req) {
    HTTP::Response res;

    res.is_file = true;
    res.file_path = req.params.at("name");

    return res;
});
```

For example:

```text
public/
├── index.html
├── images/
│   └── logo.png
└── css/
    └── style.css
```

A response with:

```cpp
res.file_path = "images/logo.png";
```

serves:

```text
./public/images/logo.png
```

Requests attempting to escape the public directory are rejected.

---

# Architecture

```text
                         Client
                            │
                            ▼
                    Accept Connection
                            │
                            ▼
                     epoll Event Loop
                            │
                            ▼
                    Connection Buffer
                            │
                            ▼
                       HTTP Parser
                            │
                            ▼
                       Thread Pool
                            │
                            ▼
                         Router
                       /        \
                      /          \
             Dynamic Routes    Static Files
                      \          /
                       \        /
                        ▼      ▼
                     Response Writer
                    /              \
                writev()         sendfile()
                    \              /
                     \            /
                          ▼
                        Client
```

AtlasHTTP separates connection readiness from request execution.

1. The main event loop accepts connections and monitors socket readiness with `epoll`.
2. Connection-specific buffers accumulate incoming TCP data.
3. Complete HTTP requests are parsed from those buffers.
4. Parsed requests are dispatched to worker threads.
5. The router selects the appropriate handler.
6. Dynamic responses are serialized using `writev()`.
7. Static files are transferred using `sendfile()`.

---

# How AtlasHTTP Works

## 1. Event-Driven Networking

AtlasHTTP uses Linux `epoll` to monitor socket readiness.

Rather than blocking on individual connections, the event loop waits for sockets that are ready for I/O and processes those events as they arrive.

This allows a relatively small number of threads to manage many concurrent connections.

## 2. Connection Buffers

TCP provides a byte stream rather than discrete messages.

A single HTTP request can therefore arrive across multiple `recv()` operations.

AtlasHTTP maintains connection-specific buffers so partial requests can be accumulated until enough data is available for parsing.

## 3. HTTP Parsing

The parser operates directly on bytes received from the socket and extracts:

* HTTP method
* Request target
* HTTP version
* Headers
* Request body
* Content length

The implementation also validates request sizes, HTTP methods, HTTP versions, and `Content-Length`.

## 4. Worker Thread Pool

Once a complete request is available, it can be dispatched to a reusable worker thread.

The thread pool uses a shared task queue rather than creating a new thread for every request.

This separates I/O readiness from request execution.

## 5. Routing

Routes are registered using lambda-based handlers.

Example:

```cpp
server.get("/users/:id", [](const HTTP::Request& req) {
    return HTTP::Response(
        200,
        req.params.at("id")
    );
});
```

Dynamic path segments are exposed through `req.params`.

## 6. Response Delivery

AtlasHTTP uses different mechanisms depending on the response type.

Dynamic responses use:

```cpp
writev()
```

to perform scatter/gather writes.

Static files use:

```cpp
sendfile()
```

to allow the kernel to transfer file data toward the socket without requiring the application to explicitly copy the file contents through a normal read/write path.

---

# Performance Engineering

AtlasHTTP was optimized through an iterative benchmark → profile → redesign process.

The goal was not simply to increase the benchmark number, but to determine which parts of the architecture actually limited performance.

### Initial Bottleneck

The original implementation achieved approximately:

```text
~5,100 req/s
```

A worker-thread model increased throughput to approximately:

```text
~21,489 req/s
```

Moving connection handling to an `epoll`-based architecture increased it further to:

```text
~43,308 req/s
```

Further architectural and implementation optimizations brought the tested dynamic-route workload to approximately:

```text
~86,898 req/s
```

### Profiling

Linux `perf` was used to inspect CPU hotspots.

Representative hot paths included:

```text
writev()
tcp_sendmsg()
tcp_sendmsg_locked()
sock_write_iter()
```

The profiling results showed that response transmission and kernel networking paths consumed a significant portion of CPU time.

This was an important finding because routing initially appeared to be a likely bottleneck.

Instead, once routing became relatively inexpensive, the cost of moving the response through the networking stack became much more significant.

This guided optimization toward:

* `epoll`
* response serialization
* `writev()`
* `sendfile()`
* reducing unnecessary memory copies
* connection and worker scheduling

---

# Core Features

## Networking

* POSIX TCP socket abstractions
* Socket creation, binding, listening, and connection handling
* Linux `epoll`
* Non-blocking sockets
* Configurable listen backlog
* Configurable worker-thread count

## HTTP

* HTTP/1.0
* HTTP/1.1
* Incremental request parsing
* GET routes
* POST routes
* Response serialization
* Keep-alive support for standard responses
* Method validation
* HTTP version validation
* Safe `Content-Length` parsing using `std::from_chars`

## Routing

* Lambda-based route handlers
* Dynamic path parameters
* Segment-based path matching

Example:

```text
/users/:id
```

with parameters exposed through:

```cpp
req.params
```

## Static File Serving

* Static files rooted at `./public`
* MIME type detection
* Path traversal protection
* `sendfile()`-based delivery

## Security

* Path traversal mitigation
* 16 KB header limit
* 1 MB body limit
* HTTP method validation
* HTTP version validation
* Safe `Content-Length` parsing

---

# Technical Challenges

Building AtlasHTTP required dealing with several low-level server-engineering problems:

* Partial TCP reads
* Socket readiness management
* Connection state
* Incremental HTTP parsing
* Thread-safe task dispatch
* Concurrent request execution
* Filesystem traversal attacks
* Response serialization
* System-call overhead
* Memory-copy overhead
* Kernel/user-space networking costs
* Benchmarking and profiling

The project was particularly useful for understanding that performance problems are often not located where the application-level code initially suggests.

---

# Project Structure

```text
.
├── .github/
│   └── workflows/
│       └── ci.yml
├── include/
│   └── AtlasHTTP/
│       ├── Server/
│       ├── Sockets/
│       └── network.hpp
├── src/
│   ├── Server/
│   └── Sockets/
├── tests/
├── examples/
├── public/
├── network.hpp
├── CMakeLists.txt
├── CMakePresets.json
├── LICENSE
└── README.md
```

`include/AtlasHTTP/` contains the public API.

`src/` contains the server and networking implementations.

`tests/` contains automated tests executed through CTest.

`examples/` contains runnable example applications.

`public/` contains static assets served by file responses.

The root-level `network.hpp` is retained as a compatibility forwarding header for older includes.

---

# Benchmark Environment

Benchmarks were performed locally against `127.0.0.1` using `wrk`.

| Component       | Configuration                      |
| --------------- | ---------------------------------- |
| CPU             | Intel Core i5-4200U @ 1.60 GHz     |
| Cores / Threads | 2 / 4                              |
| Memory          | 8 GB                               |
| OS              | Debian GNU/Linux 12                |
| Compiler        | GCC 12.2.0                         |
| Benchmark Tool  | `wrk`                              |
| Build           | `-O3 -march=native -flto -pthread` |

The results demonstrate architectural improvements and relative performance characteristics on the tested machine. They are not intended to represent production deployment performance.

---

# CI

AtlasHTTP includes a GitHub Actions workflow that:

1. Checks out the repository.
2. Configures the CMake project.
3. Builds the project.
4. Runs the automated tests.

The workflow runs on pushes and pull requests.

---

# Limitations & Future Work

The current implementation intentionally leaves several areas open for further engineering:

* Full non-blocking response buffering with `EPOLLOUT`
* Graceful server shutdown
* Chunked transfer encoding
* More advanced route lookup structures
* Expanded HTTP compliance
* URL decoding
* Additional parser validation
* Structured logging
* Broader automated test coverage
* More extensive benchmarking across workloads
* Further connection-management optimization

---

# Why This Project Exists

Most web frameworks abstract away the networking layer.

AtlasHTTP was built to expose that layer and understand the complete path from a TCP connection to an HTTP response:

```text
TCP connection
      ↓
socket readiness
      ↓
HTTP bytes
      ↓
request parser
      ↓
router
      ↓
application handler
      ↓
response serialization
      ↓
network transmission
```

The project combines systems programming with performance engineering to explore the trade-offs between concurrency, system calls, synchronization, memory movement, and kernel networking.

---

# Key Takeaways

AtlasHTTP demonstrates an end-to-end HTTP server built directly on operating-system primitives.

The project provided hands-on experience with:

* POSIX sockets
* `epoll`
* non-blocking I/O
* TCP stream handling
* HTTP parsing
* thread pools
* routing
* scatter/gather I/O
* zero-copy file delivery
* filesystem security
* Linux profiling
* benchmark-driven optimization

Most importantly, the project showed how architectural changes and profiling can reveal bottlenecks that are not obvious from application-level code.

---

# License

AtlasHTTP is licensed under the MIT License. See [`LICENSE`](LICENSE) for details.
