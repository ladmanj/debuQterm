#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QComboBox>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QLabel>
#include <QTimer>
#include <QSpinBox>
#include <QFontComboBox>
#include <QFile>
#include <QMenu>
#include <QTime>
#include <QRegularExpression>
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include "VTermWidget.h"


// PortComboBox - ComboBox wich sends signal when pop-pup
class PortComboBox : public QComboBox
{
    Q_OBJECT
public:
    using QComboBox::QComboBox;

signals:
    void popupAboutToBeShown();

protected:
    void showPopup() override {
        emit popupAboutToBeShown();
        QComboBox::showPopup();
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void refreshPorts();
    void toggleConnection();
    void readData();
    void writeData(const QByteArray &data);

    void onPortChanged(const QString &portName);
    void onRtsToggled(bool checked);
    void onDtrToggled(bool checked);
    void updateStatusLines();

private:
    void loadSettings();
    void saveSettings();

    VTermWidget *m_terminal;
    QSerialPort *m_serial;

    PortComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QComboBox *m_frameCombo;
    QPushButton *m_connectBtn;
    QPushButton *m_clearBtn;
    QSpinBox *m_fontSizeSpin;

    QFontComboBox *m_fontCombo;
    QPushButton *m_sizeBtn;

    void applyFont();

    QTimer *m_statusTimer;

    QCheckBox *m_rtsCheck;
    QCheckBox *m_dtrCheck;
    QCheckBox *m_flowCheck; // Checkbox "RTS/CTS flow-control"

    QLabel *m_ctsLabel;
    QLabel *m_dsrLabel;
    QLabel *m_dcdLabel;
    QLabel *m_riLabel;

    QLabel* createLedLabel(const QString &text);
    void setLedState(QLabel *label, bool active);

    QToolButton *m_recBtn;
    QMenu *m_recMenu;

    QFile m_logFile;
    QByteArray m_logBuffer;

    enum LogMode {
        LogBin,
        LogText,
        LogTimestamp
    };
    LogMode m_currentLogMode = LogBin;

    QProcess *m_transferProcess;
    bool m_isTransferring = false;

    enum TransferProtocol {
        ProtoZModem, // sz
        ProtoYModem, // sb (Y-Modem / Y-Modem-G)
        ProtoXModem,  // sx (X-Modem / 1k-X-Modem)
        ProtoAscii
    };
    TransferProtocol m_currentProto = ProtoZModem;

    QToolButton *m_sendBtn;
    QToolButton *m_receiveBtn;

    QTimer *m_asciiTimer;
    QStringList m_asciiLines;
    int m_asciiTotalLines;

    void sendNextAsciiLine();
    void startRecording();
    void stopRecording();
    void writeToLog(const QByteArray &data);

    void sendFile();
    void receiveFile();
    void onTransferDataFromProcess();
    void abortTransfer();

    QByteArray stripEscapeCodes(const QByteArray &data);
    void resetTransferUI();
};

#endif // MAINWINDOW_H
