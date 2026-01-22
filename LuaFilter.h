#ifndef LUAFILTER_H
#define LUAFILTER_H

#include <QString>
#include <QByteArray>

#include "lua.hpp"
/*
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
*/

class LuaFilter
{
public:
    LuaFilter();
    ~LuaFilter();

    bool loadScript(const QString &filePath);
    QString getLastError() const { return m_lastError; }

    // --- ZMĚNA API ---

    // Zpracuje příchozí data (Serial -> Terminal)
    // Hledá v Lua funkci: rx(data)
    // Pokud funkce neexistuje, vrátí data převedená na string (pass-through).
    QByteArray processRx(const QByteArray &inputData);

    // Zpracuje odchozí data (Terminal -> Serial)
    // Hledá v Lua funkci: tx(data)
    // Pokud funkce neexistuje, vrátí data nezměněná.
    QByteArray processTx(const QByteArray &inputData);

private:
    lua_State *L = nullptr;
    QString m_lastError;
    bool m_scriptLoaded = false;
};

#endif // LUAFILTER_H
