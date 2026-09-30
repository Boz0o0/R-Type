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
    if(NOT AMPERSAND_MODULE_CORE OR NOT AMPERSAND_MODULE_NET OR NOT AMPERSAND_MODULE_PHYSICS 
        OR NOT AMPERSAND_MODULE_INPUT OR NOT AMPERSAND_MODULE_RENDER OR NOT AMPERSAND_MODULE_AUDIO)
        message(FATAL_ERROR "Missing Ampersand Engine modules for the Client. Please enable the following modules: Core, Network, Physics, Input, Render, Audio.")
    endif()

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
    if(NOT AMPERSAND_MODULE_CORE OR NOT AMPERSAND_MODULE_NET OR NOT AMPERSAND_MODULE_PHYSICS)
        message(FATAL_ERROR "Missing Ampersand Engine modules for the Server. Please enable the following modules: Core, Network, Physics.")
    endif()

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
