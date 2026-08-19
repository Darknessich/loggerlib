set(LOGGER_TEST_FRAMEWORK
    framework/TestFramework.cpp
    framework/main.cpp
    utils/TempFile.cpp)

function(logger_add_tests target library)
    add_executable(${target} ${LOGGER_TEST_FRAMEWORK} ${ARGN})
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/logger/src)
    target_link_libraries(${target} PRIVATE ${library})
    logger_apply_warnings(${target})

    foreach(source IN LISTS ARGN)
        get_filename_component(suite ${source} NAME_WE)
        string(REGEX REPLACE "^test_" "" suite ${suite})
        add_test(NAME ${suite} COMMAND ${target} --suite ${suite})
        set_tests_properties(${suite} PROPERTIES
            TIMEOUT           30
            LABELS            unit
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
endfunction()

logger_add_tests(logger_tests logger_static
    test_log_level.cpp test_log_record.cpp
    test_logger_base.cpp test_file_logger.cpp)

if(TARGET app_core)
    logger_add_tests(app_tests app_core
        test_options.cpp test_input_parser.cpp test_event_queue.cpp
        test_log_worker.cpp test_console_app.cpp)
endif()

add_executable(logger_shared_smoke smoke/main.cpp utils/TempFile.cpp)
target_include_directories(logger_shared_smoke PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logger_shared_smoke PRIVATE logger_shared)
logger_apply_warnings(logger_shared_smoke)

add_test(NAME shared_link COMMAND logger_shared_smoke)
set_tests_properties(shared_link PROPERTIES TIMEOUT 30 LABELS smoke)
