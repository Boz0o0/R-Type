function(add_project_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )
    list(FILTER SOURCES EXCLUDE REGEX ".*/main\\.cpp$")
    add_library(${MODULE} STATIC ${SOURCES})
    target_include_directories(${MODULE}
        PUBLIC
            "${PROJECT_SOURCE_DIR}/include"
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_compile_features(${MODULE}
        PUBLIC
            cxx_std_20
    )
endfunction()

add_project_module(common)

target_link_libraries(common
    PUBLIC
        ampersand_core
        ampersand_net
        ampersand_physics
)

if(RTYPE_CLIENT)
    if(NOT AMPERSAND_MODULE_CORE OR NOT AMPERSAND_MODULE_NET OR NOT AMPERSAND_MODULE_PHYSICS 
        OR NOT AMPERSAND_MODULE_INPUT OR NOT AMPERSAND_MODULE_RENDER OR NOT AMPERSAND_MODULE_AUDIO
        OR NOT AMPERSAND_MODULE_SCENE OR NOT AMPERSAND_MODULE_SCRIPTING)
        message(FATAL_ERROR "Missing Ampersand Engine modules for the Client. Please enable the following modules: Core, Network, Physics, Input, Render, Audio, Scene, Scripting.")
    endif()

    add_project_module(client)
    
    target_link_libraries(client
        PUBLIC
            common
            ampersand_core
            ampersand_input
            ampersand_net
            ampersand_physics
            ampersand_render
            ampersand_audio
            ampersand_scene
            ampersand_scripting
    )
    # Like the engine's scripting module: sol2 makes too many sections for MSVC.
    if(MSVC)
        target_compile_options(client PRIVATE /bigobj)
    endif()
    
    add_executable(r-type_client
        client/main.cpp
    )
    
    target_link_libraries(r-type_client
        PRIVATE
            client
            ampersand_core
            ampersand_net
            ampersand_physics
            ampersand_input
            ampersand_render
            ampersand_audio
            ampersand_scene
            ampersand_scripting
    )

    # The client runs from its binary folder and reads game/ there.
    add_custom_target(client_game
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${PROJECT_SOURCE_DIR}/game"
            "$<TARGET_FILE_DIR:r-type_client>/game"
    )
    add_dependencies(client client_game)
endif()

if(RTYPE_SERVER)
    if(NOT AMPERSAND_MODULE_CORE OR NOT AMPERSAND_MODULE_NET OR NOT AMPERSAND_MODULE_PHYSICS)
        message(FATAL_ERROR "Missing Ampersand Engine modules for the Server. Please enable the following modules: Core, Network, Physics.")
    endif()

    add_project_module(server)
    
    target_link_libraries(server
        PUBLIC
            common
            ampersand_core
            ampersand_net
            ampersand_physics
    )
    
    add_executable(r-type_server
        server/main.cpp
    )
    
    target_link_libraries(r-type_server
        PRIVATE
            server
            ampersand_core
            ampersand_net
            ampersand_physics
    )
endif()
