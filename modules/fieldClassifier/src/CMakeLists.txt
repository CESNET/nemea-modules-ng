option(ENABLE_DEBUG_PRINTS "Enable debug printing (compile-time)" OFF)

if (ENABLE_DEBUG_PRINTS)
    add_compile_definitions(DEBUG_PRINTS)
    message(STATUS "Debug prints enabled (DEBUG_PRINTS defined)")
else()
    message(STATUS "Debug prints disabled")
endif()

add_executable(field_classifier
	main.cpp
	fieldClassifier.cpp
	plugins/geolite.cpp
)

target_link_libraries(field_classifier PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	maxminddb
)

install(TARGETS field_classifier DESTINATION ${INSTALL_DIR_BIN})
