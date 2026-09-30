cmake_minimum_required(VERSION 3.16)

project(AmpersandEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

option(AMPERSAND_MODULE_CORE "Build the Core module" ON)
option(AMPERSAND_MODULE_AUDIO "Build the Audio module" ON)
option(AMPERSAND_MODULE_INPUT "Build the Input module" ON)
option(AMPERSAND_MODULE_NET "Build the Network module" ON)
option(AMPERSAND_MODULE_PHYSICS "Build the Physics module" ON)
option(AMPERSAND_MODULE_RENDER "Build the Render module" ON)

add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()