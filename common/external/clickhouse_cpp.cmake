# clickhouse-cpp library (C++ client for ClickHouse)
include(FetchContent)

FetchContent_Declare(
	clickhouse_cpp
	GIT_REPOSITORY "https://github.com/SiskaPavel/clickhouse-cpp.git"
	GIT_TAG "65205a8"
	GIT_SHALLOW ON
)

set(DEBUG_DEPENDENCIES OFF)
set(CLICKHOUSE_INSTALL_TARGETS OFF)

add_compile_options(-Wno-pedantic -Wno-conversion -Wno-sign-conversion)

FetchContent_MakeAvailable(clickhouse_cpp)

add_library(clickhouse_cpp::client ALIAS clickhouse-cpp-lib)
