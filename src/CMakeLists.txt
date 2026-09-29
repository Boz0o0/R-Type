function(add_engine_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )

    add_library(${MODULE} STATIC ${SOURCES})

    target_include_directories(${MODULE}
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_features(${MODULE}
        PUBLIC
            cxx_std_17
    )
endfunction()

add_engine_module(common)
add_engine_module(client)
add_engine_module(server)

target_link_libraries(client
    PRIVATE
        common
)

target_link_libraries(server
    PRIVATE
        common
)

add_executable(rtype_client
    client/main.cpp
)

target_link_libraries(rtype_client
    PRIVATE
        client
)

add_executable(rtype_server
    server/main.cpp
)

target_link_libraries(rtype_server
    PRIVATE
        server
)