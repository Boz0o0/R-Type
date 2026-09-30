function(add_engine_module MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.hpp"
    )

    add_library(ampersand_${MODULE} STATIC ${SOURCES})

    target_include_directories(ampersand_${MODULE}
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    target_compile_features(ampersand_${MODULE}
        PUBLIC
            cxx_std_17
    )
endfunction()

 if(NOT AMPERSAND_MODULE_CORE)
     if(AMPERSAND_MODULE_AUDIO OR AMPERSAND_MODULE_INPUT OR AMPERSAND_MODULE_NET
        OR AMPERSAND_MODULE_PHYSICS OR AMPERSAND_MODULE_RENDER)
         message(FATAL_ERROR "AMPERSAND_MODULE_CORE must be enabled when another engine module is enabled")
     endif()
 endif()

if(AMPERSAND_MODULE_CORE)
    add_engine_module(core)
endif()

if(AMPERSAND_MODULE_AUDIO)
    add_engine_module(audio)

    target_link_libraries(ampersand_audio
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_INPUT)
    add_engine_module(input)

    target_link_libraries(ampersand_input
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_NET)
    add_engine_module(net)

    target_link_libraries(ampersand_net
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_PHYSICS)
    add_engine_module(physics)

    target_link_libraries(ampersand_physics
        PUBLIC
            ampersand_core
    )
endif()

if(AMPERSAND_MODULE_RENDER)
    add_engine_module(render)

    target_link_libraries(ampersand_render
        PUBLIC
            ampersand_core
    )
endif()