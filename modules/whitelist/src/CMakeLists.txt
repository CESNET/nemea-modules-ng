add_executable(whitelist
	main.cpp
	configParser.cpp
	csvConfigParser.cpp
	ipAddressPrefix.cpp
	whitelistRuleBuilder.cpp
	whitelist.cpp
)

target_link_libraries(whitelist PRIVATE
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
)

install(TARGETS whitelist DESTINATION ${INSTALL_DIR_BIN})
