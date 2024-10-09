# Telemetry library (C++ librabry for telemetry data collection with Fuse integration)

# Build static library
set(TELEMETRY_BUILD_SHARED OFF)
# Disable generating of installation targets
set(TELEMETRY_INSTALL_TARGETS OFF)
# Disable RPM package builder
set(TELEMETRY_PACKAGE_BUILDER OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GIT_REPO https://github.com/CESNET/telemetry.git)

FetchContent_Declare(
	telemetry
	GIT_REPOSITORY ${GIT_REPO}
	GIT_TAG v1.0.0
)

# Make sure that subproject accepts predefined build options without warnings.
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

FetchContent_MakeAvailable(telemetry)
