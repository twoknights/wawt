
include_directories(${ASIO_INCLUDE_DIR})

set(SRC ipcadapter.cpp sfmlipcadapter.cpp sfmldrawadapter.cpp sfmleventloop.cpp)

add_library(wawtsfml${LIBSUFFIX} ${SRC})
