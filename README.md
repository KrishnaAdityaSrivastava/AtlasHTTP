# AtlasHTTP

AtlasHTTP is a small C++ HTTP/1.x server framework for Linux. It implements the request lifecycle directly on top of POSIX sockets, non-blocking I/O, `epoll`, worker-thread scheduling, `writev()`, and `sendfile()` without third-party networking frameworks.

The project is intended as a systems-programming and performance-engineering codebase for understanding how an HTTP server handles sockets, request parsing, routing, response serialization, concurrency, and static-file delivery.

## Features

- POSIX TCP socket wrappers for bind, listen, and connect operations.
- Linux `epoll` event loop for connection readiness.
- Worker-thread pool for request dispatch.
- HTTP/1.0 and HTTP/1.1 request parsing for common methods.
- Lambda-based GET and POST route handlers.
- Dynamic path parameters such as `/users/:id` exposed through `HTTP::Request::params`.
- Response serialization with `writev()`.
- Static-file responses from `./public` with MIME detection, path traversal checks, and `sendfile()`.
- Request header and body size limits.

## Requirements

- Linux. AtlasHTTP uses Linux/POSIX APIs including `epoll`, sockets, `writev()`, and `sendfile()`.
- CMake 3.16 or newer.
- A C++17 compiler such as GCC or Clang.
- pthreads, provided by the system toolchain on typical Linux distributions.

No third-party C++ libraries are required.

## Repository layout

```text
.
├── .github/workflows/ci.yml      # GitHub Actions build and test workflow
├── include/AtlasHTTP/            # Public headers
│   ├── Server/                   # HTTP server, request/response, routing, and thread pool APIs
│   ├── Sockets/                  # Socket abstraction headers
│   └── network.hpp               # Umbrella networking header
├── src/                          # Library implementation files
│   ├── Server/
│   └── Sockets/
├── tests/                        # Automated tests run by CTest
├── examples/                     # Runnable example server
├── public/                       # Static web assets served by file responses
├── network.hpp                   # Compatibility forwarding header for existing includes
├── CMakeLists.txt                # Main CMake build definition
├── CMakePresets.json             # Convenience CMake configure/build presets
├── LICENSE
└── README.md
```

## Build

Configure and build with CMake:

```bash
cmake -S . -B build
cmake --build build
```

This builds:

- `atlas_http`, the reusable library target.
- `atlas_http_server`, the example server, when `ATLASHTTP_BUILD_EXAMPLES` is enabled.
- `router_tests`, when `ATLASHTTP_BUILD_TESTS` is enabled.

Useful CMake options:

```bash
cmake -S . -B build -DATLASHTTP_BUILD_EXAMPLES=ON -DATLASHTTP_BUILD_TESTS=ON
```

Release builds can be configured with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

You can also use the included presets:

```bash
cmake --preset default
cmake --build --preset default
```

## Test

Run the automated tests with CTest after building:

```bash
ctest --test-dir build --output-on-failure
```

Current tests cover route matching, dynamic path parameters, and router dispatch behavior.

## Run the example server

Build the project, then run the sample application:

```bash
./build/atlas_http_server
```

The example starts an HTTP server on port `8080` and registers:

- `GET /`, returning a small HTML response.
- `GET /data/:id`, serving a file from `./public` whose path is provided by `:id`.

Example requests:

```bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8080/data/index.html
```

Static-file responses are resolved relative to the repository's `public/` directory when the server is launched from the project root.

## Basic usage

```cpp
#include <AtlasHTTP/Server/server.hpp>

#include <csignal>
#include <netinet/in.h>
#include <sys/socket.h>

int main() {
    signal(SIGPIPE, SIG_IGN);

    HTTP::Server server(AF_INET, SOCK_STREAM, 0, 8080, INADDR_ANY, 16, 4);

    server.get("/", [](const HTTP::Request&) {
        HTTP::Response res(200, "<h1>Hello, World!</h1>");
        res.headers["Content-Type"] = "text/html";
        return res;
    });

    server.launch();
}
```

For compatibility with older code that included the repository-root header, `#include "network.hpp"` still forwards to the new public umbrella header.

## Configuration

AtlasHTTP configuration is currently done in code through the `HTTP::Server` constructor:

```cpp
HTTP::Server server(
    AF_INET,      // address family
    SOCK_STREAM,  // socket type
    0,            // protocol
    8080,         // port
    INADDR_ANY,   // interface
    16,           // listen backlog
    4             // worker thread count
);
```

Static files are served from `./public`. Launch the server from the repository root or ensure that a `public` directory exists in the process working directory.

## CI

GitHub Actions is configured in `.github/workflows/ci.yml` to run on pushes and pull requests. The workflow checks out the repository, configures CMake, builds the project, and runs CTest on Ubuntu.

## Notes and limitations

- The server is Linux-specific because it uses `epoll` and other POSIX/Linux APIs.
- The current public API is intentionally small and mostly mirrors the original project structure.
- HTTP parsing is minimal and focused on the needs of this framework; it is not a complete general-purpose HTTP implementation.

## Contributing

Contributions should keep the project dependency-light and focused on the custom HTTP server implementation. Please run the following before opening a pull request:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

AtlasHTTP is licensed under the MIT License. See [LICENSE](LICENSE) for details.
