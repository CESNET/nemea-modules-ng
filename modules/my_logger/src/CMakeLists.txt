add_executable(my_logger
	main.cpp
)

target_link_libraries(my_logger PRIVATE
	common
	unirec::unirec++
	unirec::unirec
)

install(TARGETS my_logger DESTINATION ${INSTALL_DIR_BIN})
