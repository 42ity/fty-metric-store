cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-metric-store
    VERSION 1.0.0
    DESCRIPTION "Persistance of aggregated metrics"
)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

etn_target(static ${PROJECT_NAME}-lib
    SOURCES
        src/actor_commands.cc
        src/actor_commands.h
        src/converter.cc
        src/converter.h
        src/converter2.cc
        src/converter2.h
        #src/dbstore_bench.cc
        src/fty_metric_store_server.cc
        src/fty_metric_store_server.h
        src/multi_row.cc
        src/multi_row.h
        src/persistance.cc
        src/persistance.h
    USES_PRIVATE
        mlm
        czmq
        tntdb
        cxxtools
        fty_shm
        fty_proto
        fty_common_logging
    PRIVATE
)

##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    SOURCES
        src/fty_metric_store.cc
    USES_PRIVATE
        ${PROJECT_NAME}-lib
)

##############################################################################################################

etn_test_target(${PROJECT_NAME}-lib
    SOURCES
        tests/actor_commands.cpp
        tests/converter.cpp
        tests/converter2.cpp
        tests/main.cpp
        tests/metric_store_server.cpp
    PREPROCESSOR
        -DCATCH_CONFIG_FAST_COMPILE
    SUBDIR
        tests
)

##############################################################################################################

#install resources files
set(MS_SETTINGS_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/fty/${PROJECT_NAME}")
set(MS_CONF_FILE "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME}/fty-metric-store.cfg")
set(MS_USER "bios")

configure_file("${PROJECT_SOURCE_DIR}/resources/fty-metric-store.cfg.in" "${PROJECT_BINARY_DIR}/resources/fty-metric-store.cfg" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/resources/fty-metric-store.cfg" DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/${PROJECT_NAME})

configure_file("${PROJECT_SOURCE_DIR}/resources/fty-metric-store.service.in" "${PROJECT_BINARY_DIR}/resources/fty-metric-store.service" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/resources/fty-metric-store.service" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system/)

#install cleaner old metrics
install(FILES "${PROJECT_SOURCE_DIR}/fty-metric-store-cleaner" DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})

configure_file("${PROJECT_SOURCE_DIR}/resources/fty-metric-store-cleaner.timer.in" "${PROJECT_BINARY_DIR}/resources/fty-metric-store-cleaner.timer" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/resources/fty-metric-store-cleaner.timer" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system/)

configure_file("${PROJECT_SOURCE_DIR}/resources/fty-metric-store-cleaner.service.in" "${PROJECT_BINARY_DIR}/resources/fty-metric-store-cleaner.service" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/resources/fty-metric-store-cleaner.service" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system/)

