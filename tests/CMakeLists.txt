include(GoogleTest)

function(add_engine_tests MODULE)
    if(NOT TARGET ampersand_${MODULE})
         return()
    endif()
    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/*.cpp"
    )
    add_executable(test_${MODULE}
        ${SOURCES}
    )
    target_link_libraries(test_${MODULE}
        PRIVATE
            GTest::gtest_main
            ampersand_${MODULE}
    )
    gtest_discover_tests(test_${MODULE})
endfunction()

add_engine_tests(core)
add_engine_tests(audio)
add_engine_tests(input)
add_engine_tests(net)
add_engine_tests(physics)
add_engine_tests(render)
