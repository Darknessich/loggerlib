add_library(app_core STATIC
    core/Options.cpp core/InputParser.cpp core/EventQueue.cpp
    core/LogWorker.cpp core/ConsoleApp.cpp)
target_include_directories(app_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(app_core PUBLIC Logger::Logger Threads::Threads)
logger_apply_warnings(app_core)

add_executable(logger_app src/main.cpp)
target_link_libraries(logger_app PRIVATE app_core)
logger_apply_warnings(logger_app)