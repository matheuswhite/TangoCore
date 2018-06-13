set(AUTO_GEN_MESSAGE "/* File was auto-generated, do not modify! */")
set(PRJ_MAJOR 0)
set(PRJ_MINOR 1)

set(VERSION_FILE "${PROJECT_SOURCE_DIR}/build.version")

file(READ  ${VERSION_FILE} PRJ_BUILD)
math(EXPR PRJ_BUILD "${PRJ_BUILD}+1")
file(WRITE ${VERSION_FILE} "${PRJ_BUILD}")

configure_file (
        "${PROJECT_SOURCE_DIR}/src/version.h.in"
        "${PROJECT_BINARY_DIR}/include/generated/version.h"
    )
