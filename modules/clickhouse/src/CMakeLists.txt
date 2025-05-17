add_executable(clickhouse
    main.cpp
    config.cpp
    datatype.cpp
)

target_link_libraries(clickhouse PRIVATE
    clickhouse_cpp::client
    common
    unirec::unirec++
    unirec::unirec
    trap::trap
    argparse
)

install(TARGETS clickhouse DESTINATION ${INSTALL_DIR_BIN}) 
