#include "VTermWidget.h"
#include <QPainter>
#include <QFontMetrics>
//#include <QDebug>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QClipboard>
#include <QApplication>
#include <algorithm>

VTermWidget::VTermWidget(QWidget *parent) : QWidget(parent)
{
    m_font = QFont("Consolas", 10);
    m_font.setStyleHint(QFont::Monospace);

    QFontMetrics fm(m_font);
    m_charWidth = fm.horizontalAdvance('W');
    m_charHeight = fm.height();

    if (m_charWidth <= 0) m_charWidth = 10;
    if (m_charHeight <= 0) m_charHeight = 20;

    m_vterm = vterm_new(25, 80);
    vterm_set_utf8(m_vterm, 1);

    m_vtermScreen = vterm_obtain_screen(m_vterm);
    m_vtermState = vterm_obtain_state(m_vterm);

    vterm_state_reset(m_vtermState, 1);

    // prevent uninitialized pointers
    memset(&m_callbacks, 0, sizeof(m_callbacks));

    m_callbacks.damage = &VTermWidget::onDamage;
    m_callbacks.movecursor = &VTermWidget::onMoveCursor;
    m_callbacks.sb_pushline = &VTermWidget::onPushLine;
    m_callbacks.sb_popline = &VTermWidget::onPopLine;
    m_callbacks.resize = nullptr;
    m_callbacks.moverect = nullptr;
    m_callbacks.settermprop = &VTermWidget::onSetTermProp;

    vterm_screen_set_callbacks(m_vtermScreen, &m_callbacks, this);
    vterm_output_set_callback(m_vterm, &VTermWidget::onOutput, this);

    QPalette p = palette();
    p.setColor(QPalette::Window, Qt::black);
    setPalette(p);
    setAutoFillBackground(true);

    m_blinkTimer = new QTimer(this);
    m_blinkTimer->setInterval(500); // 500 ms
    connect(m_blinkTimer, &QTimer::timeout, this, [this](){
        m_cursorBlinkState = !m_cursorBlinkState;
        update();
    });
    m_blinkTimer->start();

}

VTermWidget::~VTermWidget()
{
    vterm_free(m_vterm);
}

void VTermWidget::writeInput(const QByteArray &data)
{
    vterm_input_write(m_vterm, data.constData(), data.size());
}

void VTermWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateTermSize();
}

void VTermWidget::updateTermSize()
{
    if (m_charWidth <= 0 || m_charHeight <= 0) return;

    int cols = width() / m_charWidth;
    int rows = height() / m_charHeight;

    // Minimal size to libvterm not fail
    if (cols < 1) cols = 1;
    if (rows < 1) rows = 1;

    vterm_set_size(m_vterm, rows, cols);

    vterm_screen_flush_damage(m_vtermScreen);
    emit terminalSizeChanged(rows, cols);
}

void VTermWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setFont(m_font);

    int rows, cols;
    vterm_get_size(m_vterm, &rows, &cols);

    VTermPos cursorPos;
    vterm_state_get_cursorpos(m_vtermState, &cursorPos);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {

            VTermScreenCell cell;
            VTermPos pos = {r, c};
            vterm_screen_get_cell(m_vtermScreen, pos, &cell);

            uint32_t code = cell.chars[0];

            QColor fg = convertColor(cell.fg, true);
            QColor bg = convertColor(cell.bg, false);

            if (cell.attrs.reverse) {
                std::swap(fg, bg);
            }

            if (isSelected(r, c)) {
                std::swap(fg, bg);
                if (bg == Qt::black && fg == Qt::black) {
                    bg = Qt::lightGray;
                    fg = Qt::black;
                }
            }

            if (m_cursorVisible && m_cursorBlinkState && r == cursorPos.row && c == cursorPos.col) {
                std::swap(fg, bg);
                if (bg == Qt::black && fg == Qt::black) {
                    bg = Qt::green;
                    fg = Qt::black;
                }
            }

            QRect rect(c * m_charWidth, r * m_charHeight, m_charWidth, m_charHeight);

            if (bg != Qt::black) {
                painter.fillRect(rect, bg);
            }

            if (code != 0) {
                painter.setPen(fg);
                painter.drawText(rect, Qt::AlignCenter,
                                 QString::fromUcs4(reinterpret_cast<const char32_t*>(&code), 1));
            }
        }
    }
}

int VTermWidget::onDamage(VTermRect rect, void *user)
{
    Q_UNUSED(rect);
    VTermWidget *widget = static_cast<VTermWidget*>(user);
    widget->update();
    return 1;
}

int VTermWidget::onMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void *user)
{
    Q_UNUSED(pos);
    Q_UNUSED(oldpos);
    Q_UNUSED(visible);
    VTermWidget *widget = static_cast<VTermWidget*>(user);
    widget->update();
    return 1;
}

int VTermWidget::onPushLine(int cols, const VTermScreenCell *cells, void *user)
{
    Q_UNUSED(cols);
    Q_UNUSED(cells);
    Q_UNUSED(user);
    return 0;
}
int VTermWidget::onPopLine(int cols, VTermScreenCell *cells, void *user)
{
    Q_UNUSED(cols);

    Q_UNUSED(cells);
    Q_UNUSED(user);
    return 0;
}

QColor VTermWidget::convertColor(const VTermColor &color, bool isForeground)
{
    if (VTERM_COLOR_IS_DEFAULT_FG(&color)) return Qt::white;
    if (VTERM_COLOR_IS_DEFAULT_BG(&color)) return Qt::black;

    if (VTERM_COLOR_IS_RGB(&color)) {
        return QColor(color.rgb.red, color.rgb.green, color.rgb.blue);
    }

    if (VTERM_COLOR_IS_INDEXED(&color)) {
        uint8_t idx = color.indexed.idx;

        if (idx < 16) {
            static const QRgb ansiColors[16] = {
                0x000000, 0xAA0000, 0x00AA00, 0xAA5500, 0x0000AA, 0xAA00AA, 0x00AAAA, 0xAAAAAA, // 0-7
                0x555555, 0xFF5555, 0x55FF55, 0xFFFF55, 0x5555FF, 0xFF55FF, 0x55FFFF, 0xFFFFFF  // 8-15 (bright)
            };
            return QColor::fromRgb(ansiColors[idx]);
        }

        // Color cube 6x6x6 (16-231)
        if (idx < 232) {
            idx -= 16;
            int r = (idx / 36) * 51;
            int g = ((idx / 6) % 6) * 51;
            int b = (idx % 6) * 51;
            return QColor(r, g, b);
        }

        int gray = (idx - 232) * 10 + 8;
        return QColor(gray, gray, gray);
    }

    return isForeground ? Qt::white : Qt::black;
}

void VTermWidget::keyPressEvent(QKeyEvent *event)
{
    m_blinkTimer->start(); // Reset interval
    m_cursorBlinkState = true;
    update();

    VTermModifier mod = mapQtModifiers(event->modifiers());
    VTermKey key = mapQtKeyToVTerm(event->key());

    // 1. Special keys (Šipky, F1-F12, Enter...)
    // libvterm, handles it okay
    if (key != VTERM_KEY_NONE) {
        vterm_keyboard_key(m_vterm, key, mod);
        return;
    }

    // Legacy CTRL + A-Z
    // CTRL and a letter, computing ASCII code.
    if ((mod & VTERM_MOD_CTRL) && (event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z)) {
        // ASCII math: 'A' is 65. Ctrl+A should be 1.
        // Thus: Key - 64.
        // Qt::Key_A is 65 (0x41).
        uint32_t ctrlCode = event->key() - 64;

        vterm_keyboard_unichar(m_vterm, ctrlCode, VTERM_MOD_NONE);
        return;
    }

    // Ctrl + Space (NUL, ASCII 0)
    if ((mod & VTERM_MOD_CTRL) && event->key() == Qt::Key_Space) {
        vterm_keyboard_unichar(m_vterm, 0, VTERM_MOD_NONE);
        return;
    }

    // Normal text
    QString text = event->text();
    if (!text.isEmpty()) {
        QVector<uint32_t> ucs4 = text.toUcs4();
        for (uint32_t c : ucs4) {
            vterm_keyboard_unichar(m_vterm, c, mod);
        }
    }
}

// --- Output Callback ---
void VTermWidget::onOutput(const char *s, size_t len, void *user)
{
    VTermWidget *widget = static_cast<VTermWidget*>(user);
    emit widget->dataReadyToSend(QByteArray(s, len));
}

// --- Mapping modifiers ---
VTermModifier VTermWidget::mapQtModifiers(Qt::KeyboardModifiers modifiers)
{
    int mod = (int)VTERM_MOD_NONE;
    if (modifiers & Qt::ShiftModifier)   mod |= (int)VTERM_MOD_SHIFT;
    if (modifiers & Qt::AltModifier)     mod |= (int)VTERM_MOD_ALT;
    if (modifiers & Qt::ControlModifier) mod |= (int)VTERM_MOD_CTRL;
    return static_cast<VTermModifier>(mod);
}

// --- Mapping keys ---
VTermKey VTermWidget::mapQtKeyToVTerm(int qtKey)
{
    switch (qtKey) {
    case Qt::Key_Enter:
    case Qt::Key_Return:    return VTERM_KEY_ENTER;
    case Qt::Key_Tab:       return VTERM_KEY_TAB;
    case Qt::Key_Backspace: return VTERM_KEY_BACKSPACE;
    case Qt::Key_Escape:    return VTERM_KEY_ESCAPE;

    case Qt::Key_Up:        return VTERM_KEY_UP;
    case Qt::Key_Down:      return VTERM_KEY_DOWN;
    case Qt::Key_Left:      return VTERM_KEY_LEFT;
    case Qt::Key_Right:     return VTERM_KEY_RIGHT;

    case Qt::Key_Insert:    return VTERM_KEY_INS;
    case Qt::Key_Delete:    return VTERM_KEY_DEL;
    case Qt::Key_Home:      return VTERM_KEY_HOME;
    case Qt::Key_End:       return VTERM_KEY_END;
    case Qt::Key_PageUp:    return VTERM_KEY_PAGEUP;
    case Qt::Key_PageDown:  return VTERM_KEY_PAGEDOWN;

    case Qt::Key_F1:        return (VTermKey)VTERM_KEY_FUNCTION(1);
    case Qt::Key_F2:        return (VTermKey)VTERM_KEY_FUNCTION(2);
    case Qt::Key_F3:        return (VTermKey)VTERM_KEY_FUNCTION(3);
    case Qt::Key_F4:        return (VTermKey)VTERM_KEY_FUNCTION(4);
    case Qt::Key_F5:        return (VTermKey)VTERM_KEY_FUNCTION(5);
    case Qt::Key_F6:        return (VTermKey)VTERM_KEY_FUNCTION(6);
    case Qt::Key_F7:        return (VTermKey)VTERM_KEY_FUNCTION(7);
    case Qt::Key_F8:        return (VTermKey)VTERM_KEY_FUNCTION(8);
    case Qt::Key_F9:        return (VTermKey)VTERM_KEY_FUNCTION(9);
    case Qt::Key_F10:       return (VTermKey)VTERM_KEY_FUNCTION(10);
    case Qt::Key_F11:       return (VTermKey)VTERM_KEY_FUNCTION(11);
    case Qt::Key_F12:       return (VTermKey)VTERM_KEY_FUNCTION(12);

    default: return VTERM_KEY_NONE;
    }
}

QPoint VTermWidget::pixelsToCoords(const QPoint &pos)
{
    int col = pos.x() / m_charWidth;
    int row = pos.y() / m_charHeight;

    // Strip what's outside the screen
    int maxRows, maxCols;
    vterm_get_size(m_vterm, &maxRows, &maxCols);

    if (col < 0) col = 0;
    if (col >= maxCols) col = maxCols - 1;
    if (row < 0) row = 0;
    if (row >= maxRows) row = maxRows - 1;

    return QPoint(row, col); // returning {row, col}, not {x, y}
}

bool VTermWidget::isSelected(int row, int col)
{
    if (m_selStart.x() == -1 || m_selEnd.x() == -1) return false;

    // Normalization (start before end)
    QPoint start = m_selStart;
    QPoint end = m_selEnd;

    if (start.x() > end.x() || (start.x() == end.x() && start.y() > end.y())) {
        std::swap(start, end);
    }

    if (row < start.x() || row > end.x()) return false;

    if (row == start.x() && row == end.x()) {
        return col >= start.y() && col <= end.y();
    }

    if (row == start.x()) return col >= start.y();
    if (row == end.x())   return col <= end.y();

    return true;
}

void VTermWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_selecting = true;
        m_selStart = pixelsToCoords(event->pos());
        m_selEnd = m_selStart;
        update();
    }

#if defined(Q_OS_WINDOWS)
    else if (event->button() == Qt::RightButton) {
        pasteFromClipboard();
    }
#else
    else if (event->button() == Qt::MiddleButton) {
            pasteFromClipboard();
    }
#endif
}

void VTermWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selecting) {
        m_selEnd = pixelsToCoords(event->pos());
        update();
    }
}

void VTermWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_selecting) {
        m_selecting = false;
        copySelectionToClipboard();
    }
}

void VTermWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPoint p = pixelsToCoords(event->pos());
        // todo: find space before and after
        // selecting single letter for now
        m_selStart = p;
        m_selEnd = p;
        copySelectionToClipboard();
        update();
    }
}

void VTermWidget::copySelectionToClipboard()
{
    if (m_selStart.x() == -1 || m_selEnd.x() == -1) return;

    // Normalize
    QPoint start = m_selStart;
    QPoint end = m_selEnd;
    if (start.x() > end.x() || (start.x() == end.x() && start.y() > end.y())) {
        std::swap(start, end);
    }

    QString selectedText;

    for (int r = start.x(); r <= end.x(); r++) {
        int cStart = (r == start.x()) ? start.y() : 0;

        int rows, cols;
        vterm_get_size(m_vterm, &rows, &cols);
        int cEnd = (r == end.x()) ? end.y() : cols - 1;

        for (int c = cStart; c <= cEnd; c++) {
             VTermScreenCell cell;
             vterm_screen_get_cell(m_vtermScreen, {r, c}, &cell);
             uint32_t code = cell.chars[0];

             if (code == 0) selectedText += ' ';
             else if (code != (uint32_t)-1)
                 selectedText += QString::fromUcs4(reinterpret_cast<const char32_t*>(&code), 1);
        }

        if (r < end.x()) {
            selectedText += '\n';
        }
    }

    if (!selectedText.isEmpty()) {
        QApplication::clipboard()->setText(selectedText);
        // Todo: redraw normally after copy
    }
}

void VTermWidget::pasteFromClipboard()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString text = clipboard->text();

    if (text.isEmpty()) return;

    // text.replace("\n", "\r");

    emit dataReadyToSend(text.toUtf8());
}

void VTermWidget::clearTerminal()
{
    // Instead of \033c (Hard Reset) use:
    // \033[2J = Erase in Display (2 = whole screen)
    // \033[H  = Cursor Home (move to 0,0)

    const char *softClear = "\033[2J\033[H";

    vterm_input_write(m_vterm, softClear, 7);

    vterm_screen_flush_damage(m_vtermScreen);
    update();
    this->setFocus();
}

void VTermWidget::setTermFont(const QFont &newFont)
{
    m_font = newFont;

    QFontMetrics fm(m_font);
    m_charWidth = fm.horizontalAdvance("W");
    m_charHeight = fm.height();

    updateTermSize();

    update();
}

int VTermWidget::onSetTermProp(VTermProp prop, VTermValue *val, void *user)
{
    VTermWidget *widget = static_cast<VTermWidget*>(user);

    switch (prop) {
    case VTERM_PROP_CURSORVISIBLE:
        widget->m_cursorVisible = val->boolean;
        widget->update();
        return 1;

    case VTERM_PROP_TITLE:
        // widget->setWindowTitle(QString::fromUtf8(val->string));
        return 1;

    default:
        return 0;
    }
}
