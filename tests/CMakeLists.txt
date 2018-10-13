include(GoogleTest)

link_directories(${CMAKE_BINARY_DIR}/src/wawt
                 ${SFML_LIBRARY_DIR}
                 ${CRYPTOPP_LIBRARY_DIR})

link_libraries(gtest ${CMAKE_THREAD_LIBS_INIT})

add_executable(wawtenv_t     wawtenv_t.cpp)
add_executable(widget_t      widget_t.cpp)

target_link_libraries(wawtenv_t     wawt${LIBSUFFIX})
target_link_libraries(widget_t      wawt${LIBSUFFIX})

gtest_add_tests(TARGET wawtenv_t)
gtest_add_tests(TARGET widget_t)
