cmake_minimum_required(VERSION 3.22.1)
project(ampengine LANGUAGES C CXX)

include(FetchContent)

# Oboe für Low-Latency-Audio. Version bei Bedarf prüfen.
FetchContent_Declare(
    oboe
    URL https://github.com/google/oboe/archive/refs/tags/1.9.0.zip
)
set(OBOE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(oboe)

# TODO: NAM-Core einbinden, sobald die Lizenz geklärt ist:
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/nam-core)

add_library(ampengine SHARED amp_engine.cpp)

target_link_libraries(ampengine
    oboe
    android
    log
)
