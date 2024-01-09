# Find the Unirec++ includes and library
#
# This module defines the following IMPORTED targets:
#
#  unirec::unirec++        - The "unirec++" library, if found.
#
# This module will set the following variables in your project:
#
#  UNIRECXX_INCLUDE_DIRS - where to find <unirec++/unirec++.h>, etc.
#  UNIRECXX_LIBRARIES    - List of libraries when using unirec++.
#  UNIREC_FOUND        - True if the unirec++ library has been found.

# Use pkg-config (if available) to get the library directories and then use
# these values as hints for find_path() and find_library() functions.
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_UNIRECXX QUIET unirec++)
endif()

find_path(
    UNIRECXX_INCLUDE_DIR unirec++
    HINTS ${PC_UNIRECXX_INCLUDEDIR} ${PC_UNIRECXX_INCLUDE_DIRS}
    PATH_SUFFIXES include
)

find_library(
    UNIRECXX_LIBRARY NAMES unirec++
    HINTS ${PC_UNIRECXX_LIBDIR} ${PC_UNIRECXX_LIBRARY_DIRS}
    PATH_SUFFIXES lib lib64
)

if (PC_UNIRECXX_VERSION)
    # Version extracted from pkg-config
    set(UNIRECXX_VERSION_STRING ${PC_UNIRECXX_VERSION})
endif()

# Handle find_package() arguments (i.e. QUIETLY and REQUIRED) and set
# UNIRECXX_FOUND to TRUE if all listed variables are filled.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    UNIREC
    REQUIRED_VARS UNIRECXX_LIBRARY UNIRECXX_INCLUDE_DIR
    VERSION_VAR UNIRECXX_VERSION_STRING
)

set(UNIRECXX_INCLUDE_DIRS ${UNIRECXX_INCLUDE_DIR})
set(UNIRECXX_LIBRARIES ${UNIRECXX_LIBRARY})
mark_as_advanced(UNIRECXX_INCLUDE_DIR UNIRECXX_LIBRARY)

if (UNIREC_FOUND)
    # Create imported library with all dependencies
    if (NOT TARGET unirec::unirec++ AND EXISTS "${UNIRECXX_LIBRARIES}")
        add_library(unirec::unirec++ UNKNOWN IMPORTED)
        set_target_properties(unirec::unirec++ PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            IMPORTED_LOCATION "${UNIRECXX_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${UNIRECXX_INCLUDE_DIRS}")
    endif()
endif()

