# argparse library (python-like argparse library for C++)

FetchContent_Declare(
	argparse
	GIT_REPOSITORY "https://github.com/p-ranav/argparse.git"
	GIT_TAG        "v3.0"
)

# Make sure that subproject accepts predefined build options without warnings.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

set(ARGPARSE_INSTALL ON)
set(ARGPARSE_BUILD_TESTS OFF)
set(ARGPARSE_BUILD_SAMPLES OFF)

FetchContent_MakeAvailable(argparse)
