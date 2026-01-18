#ifndef VTERMWIDGET_H
#define VTERMWIDGET_H

#include <QWidget>
#include <QFont>
#include <QColor>
#include <QTimer>
#include <QPoint>
#include <QClipboard>
#include <QApplication>
#include <QTimer>

extern "C" {
#include <vterm.h>
}

class VTermWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VTermWidget(QWidget *parent = nullptr);
    ~VTermWidget();

    void writeInput(const QByteArray &data);
    void setTermFont(const QFont &font);

public slots:
    void clearTerminal();

signals:
    void dataReadyToSend(QByteArray data);
    void terminalSizeChanged(int rows, int cols);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    VTerm *m_vterm;
    VTermScreen *m_vtermScreen;
    VTermState *m_vtermState;

    QFont m_font;
    int m_charWidth;
    int m_charHeight;

    bool m_cursorVisible = true;
    bool m_cursorBlinkState = true;
    QTimer *m_blinkTimer;

    static int onSetTermProp(VTermProp prop, VTermValue *val, void *user);

    QColor convertColor(const VTermColor &color, bool isForeground);

    // Static callbacks for C library (must be static)
    static int onDamage(VTermRect rect, void *user);
    static int onMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int onPushLine(int cols, const VTermScreenCell *cells, void *user);
    static int onPopLine(int cols, VTermScreenCell *cells, void *user);
    static void onOutput(const char *s, size_t len, void *user);

    void updateTermSize();

    bool m_selecting = false; // Is user draging mouse
    QPoint m_selStart = {-1, -1}; // {row, col} begin
    QPoint m_selEnd = {-1, -1};   // {row, col} end

    QPoint pixelsToCoords(const QPoint &pos);

    void copySelectionToClipboard();

    bool isSelected(int row, int col);

    VTermScreenCallbacks m_callbacks;

    VTermKey mapQtKeyToVTerm(int qtKey);

    VTermModifier mapQtModifiers(Qt::KeyboardModifiers modifiers);

    void pasteFromClipboard();
};

#endif // VTERMWIDGET_H
