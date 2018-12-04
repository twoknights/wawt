set(SRC wawt.cpp widgetfactory.cpp eventrouter.cpp textentry.cpp
        scrolledlist.cpp dropdownlist.cpp ipcqueue.cpp)

add_library(wawt${LIBSUFFIX} ${SRC})
