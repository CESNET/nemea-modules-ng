add_executable(whitelist
	main.cpp
	configParser.cpp
)

target_link_libraries(whitelist PRIVATE
	common
)

install(TARGETS whitelist DESTINATION ${INSTALL_DIR_BIN})