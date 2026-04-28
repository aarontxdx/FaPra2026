cmake_minimum_required(VERSION 3.16)
project(backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Backend-Quellen
file(GLOB_RECURSE BACKEND_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

# Backend-Library
add_library(backend STATIC ${BACKEND_SOURCES})

if (WIN32)
    target_compile_definitions(backend PRIVATE _USE_MATH_DEFINES NOMINMAX)
endif()

target_include_directories(backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/deps/libosmium/include
    ${CMAKE_SOURCE_DIR}/deps/protozero/include
    ${CMAKE_SOURCE_DIR}/deps/httplib
    ${CMAKE_SOURCE_DIR}/deps/json
    ${CMAKE_SOURCE_DIR}/deps/utfcpp/source
)

find_package(ZLIB REQUIRED)
find_package(BZip2 REQUIRED)
find_package(EXPAT REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(backend PRIVATE
    ZLIB::ZLIB
    BZip2::BZip2
    EXPAT::EXPAT
    Threads::Threads
)

# Server-Executable
add_executable(server src/server.cpp)

target_link_libraries(server PRIVATE backend)
