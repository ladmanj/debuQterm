#include "LuaFilter.h"
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <exception>


int panic_handler(lua_State *L) {
    const char* msg = lua_tostring(L, -1);
    qDebug() << "LUA PANIC:" << (msg ? msg : "Unknown error");
    return 0;
}

// Wrapper pro print_status(msg, [timeout])
static int l_print_status(lua_State *L) {

    LuaFilter* self = (LuaFilter*)lua_touserdata(L, lua_upvalueindex(1));

    const char* msg = lua_tostring(L, 1);
    int timeout = luaL_optinteger(L, 2, 0);

    // 3. Emit signal
    if (self && msg) {
        emit self->statusMessageRequested(QString::fromUtf8(msg), timeout);
    }
    return 0;
}

// Wrapper for print_log(msg) - Local echo to terminal
static int l_print_log(lua_State *L) {
    LuaFilter* self = (LuaFilter*)lua_touserdata(L, lua_upvalueindex(1));

    size_t len;
    const char* msg = lua_tolstring(L, 1, &len);

    if (self && msg) {
        emit self->terminalLogRequested(QByteArray(msg, len));
    }
    return 0;
}
// ------------------------------------------------------------

LuaFilter::LuaFilter(QObject *parent) : QObject(parent)
{
    initLua();
}

void LuaFilter::initLua()
{
    closeLua();
    L = luaL_newstate();
    lua_atpanic(L, panic_handler);

    static const luaL_Reg safe_libs[] = {
        {LUA_GNAME, luaopen_base},
        {LUA_LOADLIBNAME, luaopen_package},
        {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
        {LUA_UTF8LIBNAME, luaopen_utf8},
        // {LUA_IOLIBNAME, luaopen_io},
        // {LUA_OSLIBNAME, luaopen_os},
        {LUA_DBLIBNAME, luaopen_debug},
        {NULL, NULL}
    };

    const luaL_Reg *lib;

    for (lib = safe_libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* module copy discard */
    }

    // 1. print_status
    lua_pushlightuserdata(L, this);        // Pushing 'this' on stack
    lua_pushcclosure(L, l_print_status, 1); // Creating a function with 1 "secret" variable (upvalue)
    lua_setglobal(L, "print_status");      // giving it name in lua

    // 2. print_log
    lua_pushlightuserdata(L, this);
    lua_pushcclosure(L, l_print_log, 1);
    lua_setglobal(L, "print_log");

    qDebug() << "Lua Filter initialized (API Extended)";}

LuaFilter::~LuaFilter()
{
    if (L) {
        lua_close(L);
    }
}

void LuaFilter::closeLua()
{
    if (L) {
        lua_close(L);
        L = nullptr;
    }
    m_scriptLoaded = false;
}

bool LuaFilter::loadScript(const QString &filePath)
{
    m_scriptLoaded = false;
    m_lastError.clear();

    initLua();

    qDebug() << "LUA LOAD: Loading file:" << filePath;

    if (filePath.isEmpty()) return false;
    if (!L) return false;

    // TRY-CATCH BLOCK NECCESSARY!
    try {
        int ret = luaL_dofile(L, filePath.toLocal8Bit().constData());

        if (ret != LUA_OK) {
            m_lastError = QString::fromUtf8(lua_tostring(L, -1));
            lua_pop(L, 1);
            qDebug() << "LUA LOAD ERROR:" << m_lastError;
            return false;
        }
    }
    catch (const std::exception &e) {
        qDebug() << "LUA C++ EXCEPTION:" << e.what();
        return false;
    }
    catch (...) {
        qDebug() << "LUA UNKNOWN C++ EXCEPTION";
        return false;
    }

    qDebug() << "LUA LOAD: Success!";
    m_scriptLoaded = true;
    return true;
}

QByteArray LuaFilter::processRx(const QByteArray &inputData)
{
    // If Lua isn't running, returning the input data (Pass-through)
    if (!L || !m_scriptLoaded) return inputData;

    // 1. Searching for "rx" function
    lua_getglobal(L, "rx");

    // 2. If not found (not provided by the author), returning the input
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1); // stack clean-up
        return inputData;
    }

    // 3. rx(data) function found - calling it
    lua_pushlstring(L, inputData.constData(), inputData.size());

    // checking for error, optionaly returning error message
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        size_t len;
        const char* errStr = lua_tolstring(L, -1, &len);
        QByteArray err = QByteArray(errStr, len);
        lua_pop(L, 1);
        return "[LUA RX ERROR: %1]" + err;
    }

    // 4. Passing the result to display it
    QByteArray result;
    if (lua_isstring(L, -1)) {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        result = QByteArray(str, len);
    }

    lua_pop(L, 1);
    return result;
}

QByteArray LuaFilter::processTx(const QByteArray &inputData)
{
    if (!L || !m_scriptLoaded) return inputData;

    lua_getglobal(L, "tx");

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return inputData;
    }

    lua_pushlstring(L, inputData.constData(), inputData.size());

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        qDebug() << "Lua TX Error:" << err;
        return inputData; // On error returning the original data
    }

    QByteArray result;
    if (lua_isstring(L, -1)) {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        result = QByteArray(str, (int)len);
    } else {
        // If the result is nil, returning empty array
        result = QByteArray();
    }

    lua_pop(L, 1);
    return result;
}

void LuaFilter::setGlobalInt(const QString &name, int value)
{
    if (!L) return;

    lua_pushinteger(L, value);
    lua_setglobal(L, name.toUtf8().constData());
}

QByteArray LuaFilter::triggerResize(int rows, int cols)
{
    setGlobalInt("TERM_ROWS", rows);
    setGlobalInt("TERM_COLS", cols);

    if (!L || !m_scriptLoaded) return QByteArray();

    lua_getglobal(L, "on_resize");

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return QByteArray(); // Function not found, nothing to do
    }

    lua_pushinteger(L, rows);
    lua_pushinteger(L, cols);

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        // Error in script -> write it out
        QString err = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        return "[LUA RESIZE ERROR: " + err.toUtf8() + "]";
    }

    QByteArray result;
    if (lua_isstring(L, -1)) {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        result = QByteArray(str, (int)len);
    }

    lua_pop(L, 1);
    return result;
}

QByteArray LuaFilter::triggerTick(int deltaMs)
{
    if (!L || !m_scriptLoaded) return QByteArray();

    lua_getglobal(L, "on_tick");

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return QByteArray(); // Function doesn't exist, nothing to do
    }

    lua_pushinteger(L, deltaMs);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        qDebug() << "LUA TICK ERROR:" << err;
        return QByteArray();
    }

    QByteArray result;
    if (lua_isstring(L, -1)) {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        result = QByteArray(str, (int)len);
    }

    lua_pop(L, 1);
    return result;
}
