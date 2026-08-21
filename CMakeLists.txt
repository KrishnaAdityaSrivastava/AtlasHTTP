cmake_minimum_required(VERSION 3.16)

project(AtlasHTTP VERSION 0.1.0 LANGUAGES CXX)

option(ATLASHTTP_BUILD_EXAMPLES "Build AtlasHTTP example applications" ON)
option(ATLASHTTP_BUILD_TESTS "Build AtlasHTTP tests" ON)

find_package(Threads REQUIRED)

add_library(atlas_http
    src/Server/http_io.cpp
    src/Server/router.cpp
    src/Server/server.cpp
    src/Server/thread_pool.cpp
    src/Sockets/bindSocket.cpp
    src/Sockets/connectSocket.cpp
    src/Sockets/listenSocket.cpp
    src/Sockets/socket.cpp
)

add_library(AtlasHTTP::atlas_http ALIAS atlas_http)

target_compile_features(atlas_http PUBLIC cxx_std_17)
target_include_directories(atlas_http
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(atlas_http PUBLIC Threads::Threads)

if(ATLASHTTP_BUILD_EXAMPLES)
    add_executable(atlas_http_server examples/basic_server.cpp)
    target_link_libraries(atlas_http_server PRIVATE AtlasHTTP::atlas_http)
endif()

if(ATLASHTTP_BUILD_TESTS)
    enable_testing()
    add_executable(router_tests tests/router_tests.cpp)
    target_link_libraries(router_tests PRIVATE AtlasHTTP::atlas_http)
    add_test(NAME router_tests COMMAND router_tests)
endif()
