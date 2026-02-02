#ifndef LUAFILTER_H
#define LUAFILTER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariantMap>
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

    void updateSerialLines(const QVariantMap &lines); // Volá MainWindow
    QVariantMap getLastLines() const { return m_lastLinesCache; } // Pro get_lines()
signals:
    void statusMessageRequested(QString msg, int timeout);
    void terminalLogRequested(QByteArray data);
    void setRtsRequested(bool enable);
    void setDtrRequested(bool enable);

private:
    void initLua();
    void closeLua();
    void registerFunctions();

    lua_State *L = nullptr;
    QString m_lastError;
    bool m_scriptLoaded = false;
    QVariantMap m_lastLinesCache;
};

#endif // LUAFILTER_H
