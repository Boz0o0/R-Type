cmake_minimum_required(VERSION 3.24)

project(AmpersandEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(AMPERSAND_MODULE_CORE "Build the Core module" ON)
option(AMPERSAND_MODULE_AUDIO "Build the Audio module" ON)
option(AMPERSAND_MODULE_INPUT "Build the Input module" ON)
option(AMPERSAND_MODULE_NET "Build the Network module" ON)
option(AMPERSAND_MODULE_PHYSICS "Build the Physics module" ON)
option(AMPERSAND_MODULE_RENDER "Build the Render module" ON)

option(AMPERSAND_BUILD_TESTS "Build the engine unit tests" ${PROJECT_IS_TOP_LEVEL})

include(cmake/CPM.cmake)

if(AMPERSAND_BUILD_TESTS)
    include(CTest)
endif()

if(AMPERSAND_BUILD_TESTS AND BUILD_TESTING)
    CPMAddPackage(
        NAME googletest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG v1.15.2
        OPTIONS
            "INSTALL_GTEST OFF"
            "gtest_force_shared_crt ON"
    )
endif()

if (AMPERSAND_MODULE_RENDER OR AMPERSAND_MODULE_AUDIO)
    CPMAddPackage(
        NAME SFML
        GITHUB_REPOSITORY SFML/SFML
        GIT_TAG 3.0.2
        OPTIONS
            "SFML_BUILD_EXAMPLES OFF"
            "SFML_BUILD_TEST_SUITE OFF"
            "SFML_USE_SYSTEM_DEPS OFF"
    )
endif()

if (AMPERSAND_MODULE_NET AND NOT TARGET asio)
    CPMAddPackage(
        NAME asio
        GITHUB_REPOSITORY chriskohlhoff/asio
        GIT_TAG asio-1-30-2
        DOWNLOAD_ONLY YES
    )

    find_package(Threads REQUIRED)

    add_library(asio STATIC "${asio_SOURCE_DIR}/asio/src/asio.cpp")
    target_include_directories(asio SYSTEM PUBLIC "${asio_SOURCE_DIR}/asio/include")
    target_compile_definitions(asio PUBLIC ASIO_STANDALONE ASIO_SEPARATE_COMPILATION ASIO_NO_DEPRECATED)
    target_link_libraries(asio PUBLIC Threads::Threads)

    if(WIN32)
        target_compile_definitions(asio PUBLIC _WIN32_WINNT=0x0A00)
        target_link_libraries(asio PUBLIC ws2_32 mswsock)
    endif()
endif()

add_subdirectory(src)

if(AMPERSAND_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
