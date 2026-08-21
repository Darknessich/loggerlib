add_library(collector_core STATIC
    core/Options.cpp core/LineReader.cpp core/Statistics.cpp
    core/net/Listener.cpp core/net/TcpReceiver.cpp core/net/UdpReceiver.cpp core/net/Receiver.cpp
    core/CollectorApp.cpp)
target_include_directories(collector_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(collector_core PUBLIC Logger::Logger logger_net)
logger_apply_warnings(collector_core)

add_executable(logger_collector src/main.cpp)
target_link_libraries(logger_collector PRIVATE collector_core)
logger_apply_warnings(logger_collector)
