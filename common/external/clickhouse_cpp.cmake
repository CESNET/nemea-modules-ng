# clickhouse-cpp library (C++ client for ClickHouse)

string(REPLACE "-Wsign-conversion " " " CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ")
string(REPLACE "-Wsign-conversion " " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ")
string(REPLACE "-Wsign-conversion " " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

string(REPLACE "-pedantic " " " CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ")
string(REPLACE "-pedantic " " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ")
string(REPLACE "-pedantic " " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

string(REPLACE "-Wconversion " " " CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ")
string(REPLACE "-Wconversion " " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ")
string(REPLACE "-Wconversion " " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
        clickhouse_cpp
        GIT_REPOSITORY "https://github.com/ClickHouse/clickhouse-cpp.git"
        GIT_TAG "v2.5.1"
        GIT_SHALLOW ON
)

FetchContent_MakeAvailable(clickhouse_cpp)

add_library(clickhouse_cpp::client ALIAS clickhouse-cpp-lib)
