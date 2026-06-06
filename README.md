# AtlasHTTP

**AtlasHTTP** is a high-performance HTTP/1.1 server framework built from scratch in C++17 for Linux. It implements the complete request lifecycle directly on top of POSIX sockets, `epoll`, non-blocking I/O, worker-thread scheduling, `writev()`, and `sendfile()` without relying on third-party networking frameworks.

The project was developed as a systems programming and performance engineering exercise to explore how modern web servers operate beneath framework abstractions. It focuses on connection management, event-driven networking, request parsing, routing, concurrency, response serialization, and efficient static file delivery.

---

# Highlights

* Built an HTTP server framework from scratch in C++17
* Implemented event-driven networking using Linux `epoll`
* Designed a reusable worker-thread pool for request execution
* Added dynamic route matching with path parameters
* Implemented zero-copy static file delivery using `sendfile()`
* Used `writev()` scatter/gather I/O for dynamic responses
* Added request size validation and path traversal protection
* Improved throughput from approximately **5K req/s** to **57K+ req/s** through architectural optimizations
* Benchmarked against NGINX to evaluate design decisions and scalability

---

# Performance Overview

The project was profiled using `wrk` with 4 benchmark threads and 20-second test runs. The goal was not simply to maximize throughput, but to understand how architectural decisions affect scalability and latency.

| Stage               | Architecture                   | Throughput   | Avg Latency      | p99 Latency |
| ------------------- | ------------------------------ | ------------ | ---------------- | ----------- |
| Baseline            | Single-threaded implementation | ~5,100 req/s | 9.87 ms - 154 ms | N/A         |
| Thread Pool         | Worker-thread execution        | 21,489 req/s | 23.50 ms         | 48.85 ms    |
| Epoll               | Event-driven networking        | 43,308 req/s | 18.40 ms         | 21.92 ms    |
| Epoll + Thread Pool | Final architecture             | 57,650 req/s | 20.50 ms         | 30.39 ms    |
| NGINX Reference     | Production-grade server        | 60,438 req/s | 13.09 ms         | 32.54 ms    |

The final architecture achieved throughput in the same order of magnitude as the NGINX benchmark reference while remaining a learning-oriented implementation. The comparison is intended as a performance reference rather than a claim of feature parity.

---

# Why This Project Exists

Most web frameworks abstract away networking internals, making it difficult to understand what happens between a TCP connection and an HTTP response.

AtlasHTTP was built to explore:

* Socket lifecycle management
* Event-driven server architectures
* TCP buffering and partial reads
* HTTP parsing from raw byte streams
* Concurrent request execution
* Efficient response delivery
* Static file serving techniques
* Real-world performance bottlenecks

The goal was to gain hands-on experience building the infrastructure that higher-level frameworks depend on.

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
   │
   ├── Dynamic Routes
   └── Static Files
   │
   ▼
Response Writer
(writev / sendfile)
   │
   ▼
Client
```

AtlasHTTP separates connection readiness from request execution.

1. The main event loop accepts incoming connections and monitors socket readiness through `epoll`.
2. Connection-specific buffers accumulate partial TCP data.
3. Complete HTTP requests are parsed and dispatched to worker threads.
4. Route handlers generate response objects.
5. Responses are written back using either `writev()` or `sendfile()` depending on the response type.

---

# Core Features

## Networking

* POSIX TCP socket abstractions
* Bind, listen, and connect socket wrappers
* Linux `epoll` event loop
* Non-blocking socket support
* Configurable worker-thread count
* Configurable listen backlog

## HTTP

* HTTP/1.0 support
* HTTP/1.1 support
* Request parsing from raw socket streams
* GET route handling
* POST route handling
* Response serialization
* Keep-alive support for standard responses

## Routing

* Lambda-based route handlers
* Dynamic path parameters
* Segment-based path matching

Example:

```text
/users/:id
```

Matched values are exposed through:

```cpp
req.params
```

## Static File Serving

* Secure static file serving rooted at `./public`
* MIME type detection
* Path traversal protection
* Zero-copy transfer using `sendfile()`

## Performance-Oriented Design

* Event-driven networking via `epoll`
* Non-blocking socket operations
* Worker-thread execution model
* Buffered request parsing
* `writev()` scatter/gather response writes
* `sendfile()` kernel-assisted file transfer

## Security Checks

* Path traversal mitigation
* Header size limit (16 KB)
* Body size limit (1 MB)
* HTTP method validation
* HTTP version validation
* Safe `Content-Length` parsing using `std::from_chars`

---

# Technical Challenges Solved

Building AtlasHTTP required solving several common server-engineering problems:

* Handling partial TCP reads
* Managing thousands of socket readiness events efficiently
* Designing thread-safe task dispatch
* Parsing HTTP requests incrementally
* Preventing filesystem traversal attacks
* Balancing concurrency with synchronization overhead
* Reducing response-copying costs
* Identifying performance bottlenecks through profiling

---

# Project Structure

```text
.
├── Cache/
├── Server/
├── Sockets/
├── public/
├── network.hpp
└── README.md
```

### Sockets/

Low-level networking abstractions responsible for socket creation, binding, listening, and client connections.

### Server/

Core HTTP runtime including:

* Event loop
* Request parser
* Router
* Response writer
* Thread pool

### Cache/

Small thread-safe cache implementation using:

* std::unordered_map
* std::mutex
* std::atomic

### public/

Static web assets served by AtlasHTTP. All file responses are resolved relative to this directory.
---

# Installation

## Requirements

* Linux
* C++17 Compiler
* pthread

## Build

AtlasHTTP currently uses a simple source-based build process and does not require any external dependencies or third-party networking libraries.

### Optimized Release Build

```bash
g++ -std=c++20 -O3 -march=native -flto -pthread \
Sockets/*.cpp \
Server/*.cpp \
Cache/*.cpp \
-o http_server
```

### Optimization Flags

| Flag | Purpose |
|--------|--------|
| `-O3` | Maximum compiler optimizations |
| `-march=native` | Enables CPU-specific instruction optimizations |
| `-flto` | Link Time Optimization across translation units |
| `-pthread` | Enables multithreading support |

These optimizations were used for the performance benchmarks reported in this repository.
```
## Run

./atlas_http
```

The sample application starts a server on port `8080`.

---

# Quick Start

## Create A Server

```cpp
HTTP::Server server(
    AF_INET,        // Address family
    SOCK_STREAM,    // Socket type
    0,              // Protocol (default for SOCK_STREAM)
    8080,           // Listening port
    INADDR_ANY,     // Network interface
    16,             // Listen backlog
    4               // Worker thread count
);
```

## Register A Route

```cpp
server.get("/", [](const HTTP::Request& req) {
    HTTP::Response res(
        200,
        "<h1>Hello, World!</h1>"
    );

    res.headers["Content-Type"] = "text/html";

    return res;
});
```

## Dynamic Parameters

```cpp
server.get("/users/:id", [](const HTTP::Request& req) {
    return HTTP::Response(
        200,
        req.params.at("id")
    );
});
```

## Static File Route

```cpp
server.get("/files/:name", [](const HTTP::Request& req) {
    HTTP::Response res;

    res.is_file = true;
    res.file_path = req.params.at("name");

    return res;
});
```
### Static File Serving Root

AtlasHTTP serves files exclusively from the `./public` directory.

For security reasons, file responses are resolved relative to the public root and requests outside this directory are rejected.

Project structure:

```text
.
├── public/
│   ├── index.html
│   ├── images/
│   └── css/
```

Example:

```cpp
res.is_file = true;
res.file_path = "index.html";
```

serves:

```text
./public/index.html
```

Similarly:

```cpp
res.file_path = "images/logo.png";
```

serves:

```text
./public/images/logo.png
```

Attempts to access files outside the public directory are blocked and return `403 Forbidden`.
```

## Launch

```cpp
server.launch();
```

---

# Performance Engineering

### Event-Driven Networking

AtlasHTTP uses `epoll` to monitor socket readiness efficiently, avoiding the scalability limitations of per-connection blocking models.

### Worker Thread Pool

Parsed requests are dispatched into a shared task queue and executed by reusable worker threads instead of creating new threads per request.

### Buffered Request Parsing

Each connection maintains its own buffer, allowing HTTP requests to be reconstructed correctly across partial TCP reads.

### Scatter/Gather Writes

Dynamic responses use `writev()` to reduce copying and minimize system-call overhead.

### Zero-Copy File Transfer

Static files are delivered using `sendfile()`, allowing data transfer directly from the filesystem to the socket.

---

# Areas for Further Engineering

The current implementation intentionally leaves several opportunities for future development:

* Full non-blocking write buffering with `EPOLLOUT`
* Graceful server shutdown
* Chunked transfer encoding
* Method-indexed or trie-based route lookup
* Build system integration (CMake)
* Automated tests
* Structured logging
* Improved HTTP compliance coverage
* URL decoding support
* Additional parser validation

---

# License

This project is provided for educational and experimental purposes.

---

# Key Takeaways

AtlasHTTP demonstrates how modern web servers are built beneath framework abstractions by combining low-level networking, concurrency, HTTP parsing, and performance engineering into a complete end-to-end server implementation.

It serves as both a functional HTTP framework and a practical exploration of systems-level software design.
