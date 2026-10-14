cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME sertos_stm32g070)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})
option(SERTOS_APP_INSTRUMENTED "Instrument selected application files" OFF)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Configure paths to the shared SertOS, profiler, and stream repositories
set(SERTOS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sertos" CACHE PATH "Path to the SertOS root directory")
if(SERTOS_APP_INSTRUMENTED)
    set(SERTOS_LIB_NAME "libsertos_cortex_m0plus_instrumented.a")
else()
    set(SERTOS_LIB_NAME "libsertos_cortex_m0plus.a")
endif()
set(_SERTOS_LIB_PATH_DEFAULT "${SERTOS_DIR}/lib/arm/${SERTOS_LIB_NAME}")
set(_SERTOS_LIB_PATH_NORMAL "${SERTOS_DIR}/lib/arm/libsertos_cortex_m0plus.a")
set(_SERTOS_LIB_PATH_INSTRUMENTED "${SERTOS_DIR}/lib/arm/libsertos_cortex_m0plus_instrumented.a")
if(NOT DEFINED SERTOS_LIB_PATH OR
   "${SERTOS_LIB_PATH}" STREQUAL "${_SERTOS_LIB_PATH_NORMAL}" OR
   "${SERTOS_LIB_PATH}" STREQUAL "${_SERTOS_LIB_PATH_INSTRUMENTED}" OR
   (DEFINED SERTOS_LIB_PATH_DEFAULT AND
    "${SERTOS_LIB_PATH}" STREQUAL "${SERTOS_LIB_PATH_DEFAULT}"))
    set(SERTOS_LIB_PATH "${_SERTOS_LIB_PATH_DEFAULT}" CACHE FILEPATH
        "Path to the pre-compiled SertOS Cortex-M0+ library" FORCE)
endif()
set(SERTOS_LIB_PATH_DEFAULT "${_SERTOS_LIB_PATH_DEFAULT}" CACHE INTERNAL
    "Default SertOS library path for the selected instrumentation mode" FORCE)
set(PROFILER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../profiler" CACHE PATH "Path to the profiler repository")
set(STREAM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../stream" CACHE PATH "Path to the stream repository")
set(PROFILER_PORT "2x16bit" CACHE STRING "Profiler port backend")
if(NOT EXISTS "${SERTOS_LIB_PATH}")
    message(FATAL_ERROR "Pre-compiled SertOS library not found at '${SERTOS_LIB_PATH}'. Build the Cortex-M0+ target in '${SERTOS_DIR}' first.")
endif()

set(PROFILER_LIB_PATH "${PROFILER_DIR}/lib/arm/libprofiler_cortex_m0plus.a")
if(NOT EXISTS "${PROFILER_LIB_PATH}")
    message(FATAL_ERROR "Pre-compiled profiler library not found at '${PROFILER_LIB_PATH}'. Build the Cortex-M0+ target in '${PROFILER_DIR}' first.")
endif()
if(NOT EXISTS "${PROFILER_DIR}/profiler.h")
    message(FATAL_ERROR "Profiler headers not found at '${PROFILER_DIR}'. Set PROFILER_DIR to the profiler repository.")
endif()

if(NOT EXISTS "${STREAM_DIR}/stream.h")
    message(FATAL_ERROR "Stream headers not found at '${STREAM_DIR}'. Set STREAM_DIR to the stream repository.")
endif()

if(PROFILER_PORT STREQUAL "16bit_it")
    set(PROFILER_PORT_SOURCE "${PROFILER_DIR}/port/profiler_port_16bit_it.c")
elseif(PROFILER_PORT STREQUAL "2x16bit")
    set(PROFILER_PORT_SOURCE "${PROFILER_DIR}/port/profiler_port_2x16bit.c")
else()
    message(FATAL_ERROR "Unsupported PROFILER_PORT '${PROFILER_PORT}' for this project")
endif()

set(PROFILER_TRANSPORT_SOURCE "${PROFILER_DIR}/port/profiler_transport_uart_it.c")

foreach(PROFILER_SOURCE IN ITEMS "${PROFILER_PORT_SOURCE}" "${PROFILER_TRANSPORT_SOURCE}")
    if(NOT EXISTS "${PROFILER_SOURCE}")
        message(FATAL_ERROR "Profiler backend source not found at '${PROFILER_SOURCE}'")
    endif()
endforeach()

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${SERTOS_DIR}/lib/arm"
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_demo_queue.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/profiler_port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/profiler_context_sertos.c"
    "${PROFILER_PORT_SOURCE}"
    "${PROFILER_TRANSPORT_SOURCE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_producer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_consumer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_profiler.c"
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/app"
    "${SERTOS_DIR}/inc"
    "${SERTOS_DIR}/port"
    "${SERTOS_DIR}/modules/ring_buffer"
    "${SERTOS_DIR}/modules/linked_list"
    "${SERTOS_DIR}/modules/bitmap"
    "${SERTOS_DIR}/modules/atomic"
    "${SERTOS_DIR}/modules/memory_pool"
    "${SERTOS_DIR}/modules/crc"
    "${SERTOS_DIR}/modules/fsm"
    "${PROFILER_DIR}"
    "${PROFILER_DIR}/port"
    "${STREAM_DIR}"
)

# Add project symbols (macros)
if(PROFILER_PORT STREQUAL "16bit_it")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE PROFILER_PORT_USE_16BIT_IT)
endif()

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    "${PROFILER_LIB_PATH}"
    "${SERTOS_LIB_PATH}"
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:-fopt-info-inline-missed>"
)

# Instrument application files for function call tracing.
if(SERTOS_APP_INSTRUMENTED)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SERTOS_APP_INSTRUMENTED)
    set_source_files_properties(
        "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/main.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_demo_queue.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_producer.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_consumer.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/sertos_task_profiler.c"
        PROPERTIES COMPILE_OPTIONS "-finstrument-functions"
    )
endif()

# Keep profiler callbacks out of instrumentation to prevent recursive hook calls.
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/app/profiler_port.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/profiler_context_sertos.c"
    PROPERTIES COMPILE_OPTIONS "-fno-instrument-functions"
)
