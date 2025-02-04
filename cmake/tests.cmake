# File searches for test.sh script in each module subdirectory and runs it if present

file(GLOB MODULE_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/modules ${CMAKE_SOURCE_DIR}/modules/*)
enable_testing()

add_custom_target(tests
	COMMAND ctest --output-on-failure
	VERBATIM
)

foreach(MODULE ${MODULE_DIRS})
	if (NOT IS_DIRECTORY ${CMAKE_SOURCE_DIR}/modules/${MODULE})
		continue()
	endif()
	set(TEST_SCRIPT "${CMAKE_SOURCE_DIR}/modules/${MODULE}/tests/test.sh")

	if (EXISTS ${TEST_SCRIPT})
		add_test(NAME Test${MODULE} COMMAND bash ${TEST_SCRIPT} ${CMAKE_BINARY_DIR}/modules/${MODULE}/src/${MODULE})
		add_dependencies(tests ${MODULE})
	else()
		add_custom_target(print_missing_test_for_${MODULE}
			COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --yellow --bold "No test.sh found for: ${MODULE}. Skipping..."
			VERBATIM
		)
		add_dependencies(tests print_missing_test_for_${MODULE})
	endif()
endforeach()
