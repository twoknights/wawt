link_directories(${CMAKE_BINARY_DIR}/lib
                 ${CMAKE_BINARY_DIR}/examples/adapters
                 ${SFML_LIBRARY_DIR}
                 ${CRYPTOPP_LIBRARY_DIR})

link_libraries(${CMAKE_THREAD_LIBS_INIT})

include_directories(../adapters)

add_executable(tictacdoh main.cpp stringid.cpp setupscreen.cpp)

target_link_libraries(tictacdoh   wawtsfml${LIBSUFFIX}
                                  wawt${LIBSUFFIX}
                                  sfml-graphics
                                  sfml-window
                                  sfml-system
                                  fontconfig
                                  cryptopp)
