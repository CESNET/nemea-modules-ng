# xxhash library (fast hash functions)

FetchContent_Declare(
	xxhash
	GIT_REPOSITORY "https://github.com/Cyan4973/xxHash"
	GIT_TAG        "v0.8.2"
)

FetchContent_MakeAvailable(xxhash)

set(XXHASH_SRC
	${xxhash_SOURCE_DIR}/xxhash.c
)

add_library(xxhash STATIC ${XXHASH_SRC})

target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
