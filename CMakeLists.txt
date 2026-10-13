cmake_minimum_required(VERSION 3.16)

project(Rtype
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
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

CPMAddPackage(
    NAME asio
    GITHUB_REPOSITORY chriskohlhoff/asio
    GIT_TAG asio-1-30-2
    DOWNLOAD_ONLY YES
)

if(asio_ADDED)
    find_package(Threads REQUIRED)
    add_library(asio STATIC ${asio_SOURCE_DIR}/asio/src/asio.cpp)
    target_include_directories(asio SYSTEM PUBLIC ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio PUBLIC ASIO_STANDALONE ASIO_SEPARATE_COMPILATION)
    target_link_libraries(asio PUBLIC Threads::Threads)
    if(WIN32)
        target_compile_definitions(asio PUBLIC _WIN32_WINNT=0x0A00)
        target_link_libraries(asio PUBLIC ws2_32 mswsock)
    endif()
endif()

option(RTYPE_CLIENT "Build the Client" ON)
option(RTYPE_SERVER "Build the Server" ON)

add_subdirectory(ampersand-engine)
add_subdirectory(src)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
