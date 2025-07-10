add_executable(adsnormalizer
	main.cpp
	adsnormalizer.cpp
)

target_link_libraries(adsnormalizer PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
)

install(TARGETS adsnormalizer DESTINATION ${INSTALL_DIR_BIN})
