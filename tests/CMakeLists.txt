set(LOGGER_TEST_SOURCES
    test_log_level.cpp
    test_log_record.cpp)

add_executable(logger_tests
    framework/TestFramework.cpp
    framework/main.cpp
    ${LOGGER_TEST_SOURCES})

target_include_directories(logger_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger_tests PRIVATE Logger::Logger)
logger_apply_warnings(logger_tests)

foreach(source IN LISTS LOGGER_TEST_SOURCES)
    get_filename_component(suite ${source} NAME_WE)
    string(REGEX REPLACE "^test_" "" suite ${suite})

    add_test(NAME ${suite} COMMAND logger_tests --suite ${suite})
    set_tests_properties(${suite} PROPERTIES
        TIMEOUT           30
        LABELS            unit
        ENVIRONMENT       "TZ=UTC"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

add_executable(logger_shared_smoke smoke/main.cpp)
target_link_libraries(logger_shared_smoke PRIVATE logger_shared)
logger_apply_warnings(logger_shared_smoke)

add_test(NAME shared_link COMMAND logger_shared_smoke)
set_tests_properties(shared_link PROPERTIES TIMEOUT 30 LABELS smoke)
