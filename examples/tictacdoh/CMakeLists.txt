link_directories(${CMAKE_BINARY_DIR}/lib
                 ${CMAKE_BINARY_DIR}/reference_impl
                 ${SFML_LIBRARY_DIR}
                 ${CRYPTOPP_LIBRARY_DIR})

link_libraries(${GCOVLIB} ${CMAKE_THREAD_LIBS_INIT})

include_directories(${ASIO_INCLUDE_DIR})
include_directories(../../reference_impl)

add_executable(tictacdoh stringid.cpp
                         gamescreen.cpp
                         setupscreen.cpp
                         controller.cpp
                         sfmlmain.cpp)

target_link_libraries(tictacdoh	  wawt${LIBSUFFIX}
                                  wawtsfml${LIBSUFFIX}
                                  sfml-graphics
                                  sfml-network
                                  sfml-window
                                  sfml-system
                                  fontconfig
                                  cryptopp)
