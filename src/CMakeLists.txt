function(add_project_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )
    list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
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

add_project_module(common)

if(RTYPE_CLIENT)
    add_project_module(client)
    
    target_link_libraries(client
        PRIVATE
            common
    )
    
    add_executable(rtype_client
        client/main.cpp
    )
    
    target_link_libraries(rtype_client
        PRIVATE
            client
            ampersand_core
            ampersand_net
            ampersand_physics
            ampersand_input
            ampersand_render
            ampersand_audio
    )
endif()

if(RTYPE_SERVER)
    add_project_module(server)
    
    target_link_libraries(server
        PRIVATE
            common
    )
    
    add_executable(rtype_server
        server/main.cpp
    )
    
    target_link_libraries(rtype_server
        PRIVATE
            server
            ampersand_core
            ampersand_net
            ampersand_physics
    )
endif()
