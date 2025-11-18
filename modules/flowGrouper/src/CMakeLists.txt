add_executable(flowGrouper
	main.cpp
	flowGrouper.cpp
)

target_link_libraries(flowGrouper PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	xxhash
)

install(TARGETS flowGrouper DESTINATION ${INSTALL_DIR_BIN})
