QT       += core gui widgets serialport
TARGET = debuQterm
TEMPLATE = app
CONFIG += c++17

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    LuaFilter.cpp \
    MainWindow.cpp \
    main.cpp \
    VTermWidget.cpp

HEADERS += \
    LuaFilter.h \
    MainWindow.h \
    VTermWidget.h \

RESOURCES += debuQterm.qrc
win32:RC_ICONS += res/debuQterm.ico
LIBS += -lvterm


# --- LUA INTEGRATION (PURE C++) ---

INCLUDEPATH += $$PWD/3rdparty/lua

# 1. Donutíme Qt kompilovat .c soubory jako C++
# Toto zajistí, že Lua bude používat 'try/catch' místo 'longjmp'
QMAKE_EXT_CPP += .c

# 2. Přidáme všechny původní soubory
SOURCES += $$files($$PWD/3rdparty/lua/*.c)

# 3. Vyhodíme to, co nechceme (konzole, compiler, IO, OS, INIT)
SOURCES -= $$PWD/3rdparty/lua/lua.c \
           $$PWD/3rdparty/lua/luac.c \
           $$PWD/3rdparty/lua/liolib.c \
           $$PWD/3rdparty/lua/loslib.c \
           $$PWD/3rdparty/lua/linit.c

# 4. Definujeme C++ kompatibilitu
DEFINES += LUA_USE_C89
