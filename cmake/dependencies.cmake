# Project dependencies
find_package(UNIREC++ 3.2.0 REQUIRED)
find_package(UNIREC REQUIRED)
find_package(LIBTRAP REQUIRED)
pkg_check_modules(ncurses REQUIRED IMPORTED_TARGET ncurses)
