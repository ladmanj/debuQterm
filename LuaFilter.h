#ifndef LUAFILTER_H
#define LUAFILTER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include "lua.hpp"

class LuaFilter : public QObject
{
    Q_OBJECT

public:
    explicit LuaFilter(QObject *parent = nullptr);
    ~LuaFilter();

    void reset();
    bool loadScript(const QString &filePath);
    QString getLastError() const { return m_lastError; }

    QByteArray processRx(const QByteArray &inputData);
    QByteArray processTx(const QByteArray &inputData);
    QByteArray triggerResize(int rows, int cols);
    QByteArray triggerTick(int deltaMs);

    void setGlobalInt(const QString &name, int value);

signals:
    void statusMessageRequested(QString msg, int timeout);
    void terminalLogRequested(QByteArray data);

private:
    void initLua();
    void closeLua();

    lua_State *L = nullptr;
    QString m_lastError;
    bool m_scriptLoaded = false;
};

#endif // LUAFILTER_H
