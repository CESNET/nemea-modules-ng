add_subdirectory(external)

set(LOGGER_SRC
	src/logger/logger.cpp
)

set(UNIREC_TELEMETRY_SRC
	src/unirec/unirec-telemetry.cpp
)

add_library(common OBJECT ${LOGGER_SRC} ${UNIREC_TELEMETRY_SRC})

target_link_libraries(common PUBLIC
	spdlog::spdlog
	telemetry::telemetry
	unirec::unirec++
)


target_include_directories(common PUBLIC
	include
	spdlog::spdlog
)
