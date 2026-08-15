set(LOGGER_SOURCES src/LogLevel.cpp src/LogRecord.cpp)

find_package(Threads REQUIRED)
include(GenerateExportHeader)

add_library(logger_shared SHARED ${LOGGER_SOURCES})
add_library(logger_static STATIC ${LOGGER_SOURCES})

function(logger_configure target)
    target_include_directories(${target} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:include>)
    target_include_directories(${target} PRIVATE src)
    target_link_libraries(${target} PUBLIC Threads::Threads)
    target_compile_features(${target} PUBLIC cxx_std_17)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME logger
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)
    if(LOGGER_ENABLE_WARNINGS)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

logger_configure(logger_shared)
logger_configure(logger_static)

set_target_properties(logger_shared PROPERTIES
    VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

generate_export_header(logger_shared
    BASE_NAME LOGGER
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/generated/logger/Export.h)
target_compile_definitions(logger_static PUBLIC LOGGER_STATIC_DEFINE)

if(BUILD_SHARED_LIBS)
    add_library(Logger::Logger ALIAS logger_shared)
else()
    add_library(Logger::Logger ALIAS logger_static)
endif()