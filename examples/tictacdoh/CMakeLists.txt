link_directories(${CMAKE_BINARY_DIR}/lib
                 ${CMAKE_BINARY_DIR}/reference_impl
                 ${SFML_LIBRARY_DIR}
                 ${CRYPTOPP_LIBRARY_DIR})

link_libraries(${CMAKE_THREAD_LIBS_INIT})

include_directories(../../reference_impl)

add_executable(tictacdoh main.cpp setupscreen.cpp controller.cpp stringid.cpp)

target_link_libraries(tictacdoh   wawtsfml${LIBSUFFIX}
                                  wawt${LIBSUFFIX}
                                  sfml-graphics
                                  sfml-window
                                  sfml-system
                                  fontconfig
                                  cryptopp)
