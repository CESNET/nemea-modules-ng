add_executable(clickhouse
	main.cpp
	config.cpp
	datatype.cpp
	inserter.cpp
	manager.cpp
)

target_link_libraries(clickhouse PRIVATE
	clickhouse_cpp::client
	common
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	yaml-cpp
)

install(TARGETS clickhouse DESTINATION ${INSTALL_DIR_BIN})
