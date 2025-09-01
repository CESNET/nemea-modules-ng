find_package(Boost REQUIRED)

add_executable(listDetector
	main.cpp
	configParser.cpp
	csvConfigParser.cpp
	ipAddressPrefix.cpp
	listDetector.cpp
)

target_link_libraries(listDetector PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	xxhash
	Boost::boost
)

install(TARGETS listDetector DESTINATION ${INSTALL_DIR_BIN})
