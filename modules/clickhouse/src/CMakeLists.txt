add_executable(clickhouse
	main.cpp
	inserter.cpp
	config.cpp
	manager.cpp
	datatype.cpp
)

target_link_libraries(clickhouse PRIVATE
	clickhouse_cpp::client
	telemetry::telemetry
	telemetry::appFs
	common
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
)

install(TARGETS clickhouse DESTINATION ${INSTALL_DIR_BIN})
