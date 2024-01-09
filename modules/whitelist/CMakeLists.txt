add_executable(whitelist
	main.cpp
	configParser.cpp
	csvConfigParser.cpp
)

target_link_libraries(whitelist PRIVATE
	common
	rapidcsv
)

install(TARGETS whitelist DESTINATION ${INSTALL_DIR_BIN})