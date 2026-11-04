include(GoogleTest)

function(add_engine_tests MODULE)
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
    )

    add_executable(tests_${MODULE}
        ${SOURCES}
    )

    target_link_libraries(tests_${MODULE}
        PRIVATE
            GTest::gtest_main
            ${MODULE}
    )

    gtest_discover_tests(tests_${MODULE})
endfunction()

add_engine_tests(common)

if(RTYPE_CLIENT)
    add_engine_tests(client)
    # The tests load the real game/ files of the repository.
    target_compile_definitions(tests_client
        PRIVATE RTYPE_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(tests_client PRIVATE /bigobj)
    endif()
endif()

if(RTYPE_SERVER)
    add_engine_tests(server)
endif()