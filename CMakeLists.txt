cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-metric-store
    VERSION 1.0.0
    DESCRIPTION "Persistance for metrics"
)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################

etn_target(exe ${PROJECT_NAME}
    SOURCES
        src/actor_commands.cc
        src/converter.cc
        src/multi_row.cc
        src/persistance.cc
        #src/dbstore_bench.cc
        src/fty_metric_store.cc
        src/fty_metric_store_server.cc
    INCLUDE_DIRS
        include
    USES_PRIVATE
        czmq
        fty_common_logging
        fty_proto
        cxxtools
        tntdb
        fty_shm
        libmlm
)

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

if (BUILD_TESTING)

    etn_test(${PROJECT_NAME}-test
        SOURCES
            tests/main.cc
            src/actor_commands.cc
            src/converter.cc
            src/multi_row.cc
            src/persistance.cc
            src/fty_metric_store_server.cc
            src/fty_metric_store_private_selftest.cc
        INCLUDE_DIRS
            include
        PREPROCESSOR -DCATCH_CONFIG_FAST_COMPILE
        USES
            czmq
            fty_common_logging
            fty_proto
            cxxtools
            tntdb
            fty_shm
            libmlm
            Catch2::Catch2
            stdc++fs
            log4cplus
    )

    #copy selftest-ro for test
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/selftest-ro DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    file(MAKE_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/selftest-rw)

    #enable coverage
    etn_coverage(${PROJECT_NAME}-test)

    #copy selftest-ro for test coverage
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/selftest-ro DESTINATION ${CMAKE_BINARY_DIR})
    file(MAKE_DIRECTORY  ${CMAKE_BINARY_DIR}/selftest-rw)

endif()