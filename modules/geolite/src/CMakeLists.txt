add_executable(geolite
	main.cpp
	geolite.cpp
)

target_link_libraries(geolite PRIVATE
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

install(TARGETS geolite DESTINATION ${INSTALL_DIR_BIN})
