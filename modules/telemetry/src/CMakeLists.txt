add_executable(telemetry
	main.cpp
	timer.cpp
)

target_link_libraries(telemetry PRIVATE
	common
	unirec::unirec++
	unirec::unirec
	argparse
)

install(TARGETS telemetry DESTINATION ${INSTALL_DIR_BIN})
