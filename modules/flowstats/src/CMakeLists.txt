add_executable(flowstats
	main.cpp
	flowstats.cpp
)

target_link_libraries(flowstats PRIVATE
	common
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
)

install(TARGETS flowstats DESTINATION ${INSTALL_DIR_BIN})
