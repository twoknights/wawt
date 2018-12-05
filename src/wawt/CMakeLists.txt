set(SRC
    wawt.cpp widgetfactory.cpp textentry.cpp scrolledlist.cpp dropdownlist.cpp
    eventrouter.cpp 
    ipcmessage.cpp ipcsession.cpp ipcqueue.cpp)

add_library(wawt${LIBSUFFIX} ${SRC})
