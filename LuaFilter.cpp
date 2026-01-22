#include "LuaFilter.h"
#include <QDebug>
#include <QFileInfo>
#include <QCoreApplication>
#include <exception>


// Funkce pro případ nouze (kdyby Lua chtěla ukončit program)
int panic_handler(lua_State *L) {
    const char* msg = lua_tostring(L, -1);
    qDebug() << "LUA PANIC (CRITICAL):" << (msg ? msg : "Unknown error");
    return 0; // Návratem ukončíme aplikaci, ale aspoň uvidíme log
}

LuaFilter::LuaFilter()
{
    // 1. Vytvoření virtuálního stroje Lua
    L = luaL_newstate();
    lua_atpanic(L, panic_handler); // Registrace záchrany

    // 2. Otevření standardních knihoven (math, string, table, bit32...)
    // Bez toho by ve skriptu nešlo nic dělat.
    //luaL_openlibs(L);

    static const luaL_Reg safe_libs[] = {
        {LUA_GNAME, luaopen_base},
        {LUA_LOADLIBNAME, luaopen_package},
        {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table},
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
        {LUA_UTF8LIBNAME, luaopen_utf8},
        // {LUA_IOLIBNAME, luaopen_io},  <-- TOTO ZABÍJÍ SÉRIOVÝ PORT
        // {LUA_OSLIBNAME, luaopen_os},  <-- TOTO ČASTO TAKY
        {LUA_DBLIBNAME, luaopen_debug},
        {NULL, NULL}
    };

    const luaL_Reg *lib;
    // Smyčka, která načte knihovny jednu po druhé (stejně jako to dělá openlibs interně)
    for (lib = safe_libs; lib->func; lib++) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1);  /* vyhodit kopii modulu ze zásobníku */
    }

    qDebug() << "Lua Filter initialized (Safe Mode)";
}

LuaFilter::~LuaFilter()
{
    if (L) {
        lua_close(L);
    }
}

bool LuaFilter::loadScript(const QString &filePath)
{
    m_scriptLoaded = false;
    m_lastError.clear();

    qDebug() << "LUA LOAD: Loading file:" << filePath;

    if (filePath.isEmpty()) return false;
    if (!L) return false;

    // TRY-CATCH BLOK JE NYNÍ NUTNÝ!
    try {
        // Použijeme standardní load
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
    // Pokud Lua neběží, vrátíme data jako prostý text (Pass-through)
    if (!L || !m_scriptLoaded) return inputData;

    // 1. Hledáme funkci "rx"
    lua_getglobal(L, "rx");

    // 2. Pokud to NENÍ funkce (autor ji nenapsal), vrátíme původní data
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1); // Uklidit stack
        return inputData;
    }

    // 3. Voláme funkci rx(data)
    lua_pushlstring(L, inputData.constData(), inputData.size());

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        size_t len;
        const char* errStr = lua_tolstring(L, -1, &len);
        QByteArray err = QByteArray(errStr, len);
        lua_pop(L, 1);
        return "[LUA RX ERROR: %1]" + err;
    }

    // 4. Zpracování výsledku (očekáváme string pro zobrazení)
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
    // Pokud Lua neběží, vrátíme původní data
    if (!L || !m_scriptLoaded) return inputData;

    // 1. Hledáme funkci "tx"
    lua_getglobal(L, "tx");

    // 2. Pokud neexistuje, posíláme originál
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return inputData;
    }

    // 3. Voláme funkci tx(data)
    lua_pushlstring(L, inputData.constData(), inputData.size());

    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        QString err = QString::fromUtf8(lua_tostring(L, -1));
        lua_pop(L, 1);
        qDebug() << "Lua TX Error:" << err;
        return inputData; // Při chybě pošleme raději původní data (nebo nic?)
    }

    // 4. Zpracování výsledku (očekáváme binární data pro odeslání)
    QByteArray result;
    if (lua_isstring(L, -1)) {
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        result = QByteArray(str, (int)len);
    } else {
        // Pokud funkce vrátila nil (třeba chce zahodit data), pošleme prázdné pole
        result = QByteArray();
    }

    lua_pop(L, 1);
    return result;
}
