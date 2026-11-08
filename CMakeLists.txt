cmake_minimum_required(VERSION 3.24)

project(Rtype
    VERSION 1.0.0
    LANGUAGES C CXX
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

CPMAddPackage(
    NAME AmpersandEngine
    GITHUB_REPOSITORY Boz0o0/Ampersand-Engine
    GIT_TAG dfdbc9b1308ca57eb1c727a5599e2ddc9a1cb018
)
CPMAddPackage(
    NAME sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
    DOWNLOAD_ONLY YES
)
add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 SYSTEM PUBLIC ${sqlite3_SOURCE_DIR})

CPMAddPackage(NAME monocypher
  GITHUB_REPOSITORY LoupVaillant/Monocypher
  GIT_TAG 4.0.2
  DOWNLOAD_ONLY YES)
add_library(monocypher STATIC ${monocypher_SOURCE_DIR}/src/monocypher.c)
target_include_directories(monocypher SYSTEM PUBLIC ${monocypher_SOURCE_DIR}/src)

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
