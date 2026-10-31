cmake_minimum_required(VERSION 3.24)

project(Rtype
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include(cmake/CPM.cmake)

include(CTest)

if(BUILD_TESTING)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.15.2
        OPTIONS
            "INSTALL_GTEST OFF"
            "gtest_force_shared_crt ON"
    )
endif()

option(RTYPE_CLIENT "Build the Client" ON)
option(RTYPE_SERVER "Build the Server" ON)

set(AMPERSAND_ENGINE_VERSION 0.5.0)
# v0.5.0 is not released yet: until it is, build the head of the engine PR #35
# (Lua scripting), then go back to GIT_TAG v${AMPERSAND_ENGINE_VERSION}.
CPMAddPackage(
    NAME AmpersandEngine
    GITHUB_REPOSITORY Boz0o0/Ampersand-Engine
    GIT_TAG e5ca3484419ca9a2fb212ab3fa069a8ee8a31489
)

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
