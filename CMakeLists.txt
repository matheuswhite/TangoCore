include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(Tango)

include_directories(
    "${PROJECT_BINARY_DIR}/include/generated"
    "${PROJECT_SOURCE_DIR}/include"
    )

# Generate version file
include(build.version.cmake NO_POLICY_SCOPE)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Schedule a new version file generation after every build
add_custom_command(
    TARGET app
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
    -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
    -P ${PROJECT_SOURCE_DIR}/build.version.cmake
    )
