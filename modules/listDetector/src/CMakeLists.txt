add_executable(listDetector
	main.cpp
	configParser.cpp
	csvConfigParser.cpp
	ipAddressPrefix.cpp
	rule.cpp
	ruleBuilder.cpp
	listDetector.cpp
	ipAddressFieldMatcher.cpp
	fieldsMatcher.cpp
	rulesMatcher.cpp
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
)

install(TARGETS listDetector DESTINATION ${INSTALL_DIR_BIN})
