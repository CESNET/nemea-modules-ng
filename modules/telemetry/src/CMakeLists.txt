add_executable(telemetry_mod
	main.cpp
	timer.cpp
)

target_link_libraries(telemetry_mod PRIVATE
	common
	telemetry::telemetry
	unirec::unirec++
	unirec::unirec
	argparse
)

install(TARGETS telemetry_mod DESTINATION ${INSTALL_DIR_BIN})
