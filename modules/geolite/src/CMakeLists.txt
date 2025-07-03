add_executable(geolite
	main.cpp
)

install(TARGETS geolite DESTINATION ${INSTALL_DIR_BIN})
