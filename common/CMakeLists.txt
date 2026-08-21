add_library(logger_common INTERFACE)
target_include_directories(logger_common INTERFACE ${CMAKE_SOURCE_DIR})

add_library(logger_net OBJECT net/Socket.cpp net/Address.cpp)
target_link_libraries(logger_net PUBLIC logger_common)
set_target_properties(logger_net PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET     hidden
    VISIBILITY_INLINES_HIDDEN ON)
target_compile_features(logger_net PUBLIC cxx_std_17)
logger_apply_warnings(logger_net)
