add_executable(telemetry_mod
	main.cpp
	timer.cpp
	cursesPrint.cpp
)

target_link_libraries(telemetry_mod PRIVATE
	curses
	telemetry::telemetry
	telemetry::appFs
	common
	unirec::unirec++
	unirec::unirec
	argparse
)

install(TARGETS telemetry_mod DESTINATION ${INSTALL_DIR_BIN})
