add_executable(deduplicator
	main.cpp
	deduplicator.cpp
)

target_link_libraries(deduplicator PRIVATE
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

install(TARGETS deduplicator DESTINATION ${INSTALL_DIR_BIN})
