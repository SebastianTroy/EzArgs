TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle qt

DEFINES+=CATCH_CONFIG_MAIN

SOURCES += \
    testMain.cpp

HEADERS += \
    Catch.h \
    EzArgs.h

DISTFILES += \
    README.md
