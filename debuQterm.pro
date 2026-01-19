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

RESOURCES += debuQterm.qrc
win32:RC_ICONS += res/debuQterm.ico
LIBS += -lvterm
