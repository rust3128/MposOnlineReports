QT -= gui
QT += sql network
CONFIG += core
CONFIG += c++11 console

CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
include(../QtWebApp/httpserver/httpserver.pri)

include(../QtWebApp/templateengine/templateengine.pri)

include(../QtWebApp/logging/logging.pri)


SOURCES += \
        global.cpp \
        main.cpp \
        objectslist.cpp \
        requestmapper.cpp \
        shiftreports.cpp \
        shiftslist.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../docroot/login.html \
    ../docroot/objectslist.html \
    ../docroot/shiftreport.html \
    ../docroot/shiftslist.html \
    ../etc/MPosOnlineReports.ini

HEADERS += \
    global.h \
    objectslist.h \
    requestmapper.h \
    shiftreports.h \
    shiftslist.h
