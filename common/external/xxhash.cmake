# XXHash library (C librabry that provides hash functions)

set(GIT_REPO https://github.com/Cyan4973/xxHash)

FetchContent_Declare(
	xxhash
	GIT_REPOSITORY ${GIT_REPO}
	GIT_TAG v0.8.2
)

FetchContent_MakeAvailable(xxhash)

set(XXHASH_SRC
	${xxhash_SOURCE_DIR}/xxhash.c
)

add_library(xxhash STATIC ${XXHASH_SRC})

target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
