link_directories(${CMAKE_BINARY_DIR}/lib
                 ${CMAKE_BINARY_DIR}/reference_impl
                 ${SFML_LIBRARY_DIR})

link_libraries(${GCOVLIB} ${CMAKE_THREAD_LIBS_INIT})

include_directories(../../reference_impl)

add_executable(factorysamples sfmlmain.cpp)

target_link_libraries(factorysamples wawt${LIBSUFFIX}
                                     wawtsfml${LIBSUFFIX}
                                     sfml-graphics
                                     sfml-window
                                     sfml-system
                                     fontconfig)
