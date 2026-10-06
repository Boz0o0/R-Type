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
endif()

if(RTYPE_SERVER)
    add_engine_tests(server)
endif()