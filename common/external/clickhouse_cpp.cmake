# clickhouse-cpp library (C++ client for ClickHouse)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
	clickhouse_cpp
	GIT_REPOSITORY "https://github.com/ClickHouse/clickhouse-cpp.git"
	GIT_TAG "v2.5.1"
	GIT_SHALLOW ON
)

FetchContent_MakeAvailable(clickhouse_cpp)

add_library(clickhouse_cpp::client ALIAS clickhouse-cpp-lib)
