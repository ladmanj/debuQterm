QT       += core gui widgets serialport
TARGET = debuQterm
TEMPLATE = app
CONFIG += c++17 console

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    MainWindow.cpp \
    main.cpp \
    VTermWidget.cpp

HEADERS += \
    MainWindow.h \
    VTermWidget.h \

LIBS += -lvterm
