add_executable(flowscatter
	main.cpp
	flowScatter.cpp
)

target_link_libraries(flowscatter PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	xxhash
)

install(TARGETS flowscatter DESTINATION ${INSTALL_DIR_BIN})
