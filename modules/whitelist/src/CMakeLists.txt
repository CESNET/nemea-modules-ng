add_executable(whitelist
	main.cpp
	configParser.cpp
	csvConfigParser.cpp
	ipAddressPrefix.cpp
	whitelistRule.cpp
	whitelistRuleBuilder.cpp
	whitelist.cpp
)

target_link_libraries(whitelist PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
)

install(TARGETS whitelist DESTINATION ${INSTALL_DIR_BIN})
