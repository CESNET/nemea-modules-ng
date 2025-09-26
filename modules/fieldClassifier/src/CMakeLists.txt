add_executable(field_classifier
	main.cpp
	geolite.cpp
	templateCreator.cpp
	fieldProcessor.cpp
	ipClassifier.cpp
	sniClassifier.cpp
	asn.cpp

)

target_link_libraries(field_classifier PRIVATE
	telemetry::telemetry
	telemetry::appFs
	common
	rapidcsv
	unirec::unirec++
	unirec::unirec
	trap::trap
	argparse
	maxminddb
)

install(TARGETS field_classifier DESTINATION ${INSTALL_DIR_BIN})
