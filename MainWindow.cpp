/* MainWindow.cpp*/
#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QGroupBox>
#include <QDebug>
#include <QFontComboBox>
#include <QSettings>
#include <QCloseEvent>
#include <QStyleFactory>
#include <QActionGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_serial(new QSerialPort(this))
{
    m_asciiTimer = new QTimer(this);
    m_asciiTimer->setInterval(50);
    connect(m_asciiTimer, &QTimer::timeout, this, &MainWindow::sendNextAsciiLine);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QHBoxLayout *controlsLayout = new QHBoxLayout();

    m_portCombo = new PortComboBox();

    m_portCombo->setMaxVisibleItems(15);

    m_baudCombo = new QComboBox();

    // Frame selection (Data Bits / Parity / Stop Bits)
    m_frameCombo = new QComboBox();

    // Standard modern configuration
    m_frameCombo->addItem("8N1 (Standard)", "8N1");

    // 7-bit configurations (Vintage/ASCII)
    m_frameCombo->addItem("7E1 (7bit Even 1stop)", "7E1");
    m_frameCombo->addItem("7O1 (7bit Odd  1stop)",  "7O1");
    m_frameCombo->addItem("7O1 (7bit None 1stop)",  "7N1");
    m_frameCombo->addItem("7N2 (Teletype)",  "7N2");

    // 8-bit variations (Industrial/Slow)
    m_frameCombo->addItem("8E1 (8bit Even 1stop)", "8E1");
    m_frameCombo->addItem("8O1 (8bit Odd  1stop)",  "8O1");
    m_frameCombo->addItem("8N2 (8bit None 2stop)",   "8N2");

    m_frameCombo->setToolTip("Frame format: Data bits / Parity / Stop bits");

    m_connectBtn = new QPushButton("Connect");
    m_clearBtn = new QPushButton("Clear");

    m_connectBtn->setFocusPolicy(Qt::NoFocus);
    m_clearBtn->setFocusPolicy(Qt::NoFocus);

    m_clearBtn->setIcon(QIcon::fromTheme("edit-clear"));

    m_recBtn = new QToolButton();
    m_recBtn->setText("REC");
    m_recBtn->setMinimumWidth(80);
    m_recBtn->setCheckable(true);
    m_recBtn->setFocusPolicy(Qt::NoFocus);
    m_recBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_recBtn->setPopupMode(QToolButton::MenuButtonPopup);

    m_fontCombo = new QFontComboBox();
    m_fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    m_fontCombo->setFocusPolicy(Qt::NoFocus);

    m_recMenu = new QMenu(this);

    QAction *actBin = m_recMenu->addAction("Raw data");
    actBin->setCheckable(true);
    actBin->setChecked(true); // Default

    QAction *actText = m_recMenu->addAction("Text only");
    actText->setCheckable(true);

    QAction *actLog = m_recMenu->addAction("Log (Timestamp)");
    actLog->setCheckable(true);

    QActionGroup *group = new QActionGroup(this);
    group->addAction(actBin);
    group->addAction(actText);
    group->addAction(actLog);

    m_recBtn->setMenu(m_recMenu);

    connect(actBin, &QAction::triggered, this, [this](){ m_currentLogMode = LogBin; });
    connect(actText, &QAction::triggered, this, [this](){ m_currentLogMode = LogText; });
    connect(actLog, &QAction::triggered, this, [this](){ m_currentLogMode = LogTimestamp; });

    connect(m_recBtn, &QPushButton::clicked, this, [this](bool checked){
        if (checked) {
            startRecording();
        } else {
            stopRecording();
        }
    });

    m_sendBtn = new QToolButton();
    m_sendBtn->setText("Send (Z-Modem)"); // Default
    m_sendBtn->setPopupMode(QToolButton::MenuButtonPopup);
    m_sendBtn->setFocusPolicy(Qt::NoFocus);
    m_sendBtn->setMinimumWidth(180);

    QMenu *sendMenu = new QMenu(this);

    QAction *actZ = sendMenu->addAction("Z-Modem");
    QAction *actY = sendMenu->addAction("Y-Modem");
    QAction *actX = sendMenu->addAction("X-Modem");
    QAction *actAscii = sendMenu->addAction("Text (Line-by-Line)");

    connect(actZ, &QAction::triggered, this, [this](){
        m_currentProto = ProtoZModem;
        m_sendBtn->setText("Send (Z-Modem)");
    });

    connect(actY, &QAction::triggered, this, [this](){
        m_currentProto = ProtoYModem;
        m_sendBtn->setText("Send (Y-Modem)");
    });

    connect(actX, &QAction::triggered, this, [this](){
        m_currentProto = ProtoXModem;
        m_sendBtn->setText("Send (X-Modem)");
    });

    connect(actAscii, &QAction::triggered, this, [this](){
        m_currentProto = ProtoAscii;
        m_sendBtn->setText("Send (Text)");
    });

    m_sendBtn->setMenu(sendMenu);

    connect(m_sendBtn, &QToolButton::clicked, this, &MainWindow::sendFile);

    m_receiveBtn = new QToolButton();

    m_receiveBtn->setText("Receive (Z-Modem)"); // Default
    m_receiveBtn->setPopupMode(QToolButton::MenuButtonPopup);

    m_receiveBtn->setFocusPolicy(Qt::NoFocus);

    QMenu *recvMenu = new QMenu(this);

    QAction *actRecvZ = recvMenu->addAction("Z-Modem (Auto)");
    QAction *actRecvY = recvMenu->addAction("Y-Modem (Batch)");
    QAction *actRecvX = recvMenu->addAction("X-Modem (1 file)");

    connect(actRecvZ, &QAction::triggered, this, [this](){
        m_currentProto = ProtoZModem;
        m_receiveBtn->setText("Receive (Z-Modem)");
    });
    connect(actRecvY, &QAction::triggered, this, [this](){
        m_currentProto = ProtoYModem;
        m_receiveBtn->setText("Receive (Y-Modem)");
    });
    connect(actRecvX, &QAction::triggered, this, [this](){
        m_currentProto = ProtoXModem;
        m_receiveBtn->setText("Receive (X-Modem)");
    });

    m_receiveBtn->setMenu(recvMenu);

    connect(m_receiveBtn, &QToolButton::clicked, this, &MainWindow::receiveFile);

    m_fontSizeSpin = new QSpinBox();
    m_fontSizeSpin->setRange(8, 36);
    m_fontSizeSpin->setValue(10);
    m_fontSizeSpin->setSuffix(" pt");
    m_fontSizeSpin->setFocusPolicy(Qt::NoFocus);
    // ----------------------------------

    QList<qint32> baudRates = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 921600};
    for (qint32 baud : baudRates) {
        m_baudCombo->addItem(QString::number(baud), baud);
    }
    m_baudCombo->setCurrentText("115200"); // Default

    // --- CHECKBOX PRO HW FLOW CONTROL ---
    m_flowCheck = new QCheckBox("Flow");
    m_flowCheck->setToolTip("Enable RTS/CTS hardware Flow Control.\n"
                            "Manual control of RTS will be disabled.");

    // scripting
    m_luaTimer = new QTimer(this);
    m_luaTimer->setInterval(100); // 100 ms period
    connect(m_luaTimer, &QTimer::timeout, this, &MainWindow::onLuaTick);
    m_luaTimer->start();
    m_luaStatsTimer.start();

    connect(&m_lua, &LuaFilter::statusMessageRequested, this, [this](QString msg, int timeout){
        statusBar()->showMessage(msg, timeout);
    });

    connect(&m_lua, &LuaFilter::terminalLogRequested, this, [this](QByteArray data){
        m_terminal->writeInput(data);
    });

    m_btnLoadScript = new QPushButton("Script...");
    m_btnLoadScript->setFocusPolicy(Qt::NoFocus);
    connect(m_btnLoadScript, &QPushButton::clicked, this, &MainWindow::onScriptButtonClicked);

    m_scriptCheck = new QCheckBox("Filter");
    m_scriptCheck->setToolTip("Activate Lua filter");
    m_scriptCheck->setFocusPolicy(Qt::NoFocus);


    controlsLayout->addWidget(m_portCombo);

    controlsLayout->addWidget(m_baudCombo);
    controlsLayout->addWidget(m_frameCombo);
    controlsLayout->addWidget(m_flowCheck);
    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_connectBtn);

    controlsLayout->addStretch();

    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_recBtn);

    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_sendBtn);
    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_receiveBtn);

    controlsLayout->addSpacing(10);
    controlsLayout->addWidget(m_btnLoadScript);
    controlsLayout->addWidget(m_scriptCheck);

    mainLayout->addLayout(controlsLayout);

    m_terminal = new VTermWidget();
    mainLayout->addWidget(m_terminal, 1);

    // initial size expectation, will be updated soon
    m_termRows = 24;
    m_termCols = 80;


    QHBoxLayout *statusLayout = new QHBoxLayout();

    QGroupBox *ctrlBox = new QGroupBox("Outputs");
    ctrlBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QHBoxLayout *ctrlLayout = new QHBoxLayout(ctrlBox);
    ctrlLayout->setContentsMargins(5, 5, 5, 5);

    m_rtsCheck = new QCheckBox("RTS");
    m_dtrCheck = new QCheckBox("DTR");

    m_rtsCheck->setFocusPolicy(Qt::NoFocus);
    m_dtrCheck->setFocusPolicy(Qt::NoFocus);

    m_rtsCheck->setChecked(false);
    m_dtrCheck->setChecked(false);

    ctrlLayout->addWidget(m_rtsCheck);
    ctrlLayout->addWidget(m_dtrCheck);

    QGroupBox *statBox = new QGroupBox("Inputs");
    statBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QHBoxLayout *statLayout = new QHBoxLayout(statBox);
    statLayout->setContentsMargins(5, 5, 5, 5);

    m_ctsLabel = createLedLabel("CTS");
    m_dsrLabel = createLedLabel("DSR");
    m_dcdLabel = createLedLabel("DCD");
    m_riLabel  = createLedLabel("RI");

    statLayout->addWidget(m_ctsLabel);
    statLayout->addWidget(m_dsrLabel);
    statLayout->addWidget(m_dcdLabel);
    statLayout->addWidget(m_riLabel);


    statusLayout->addWidget(ctrlBox);
    statusLayout->addStretch();

    QGroupBox *textBox = new QGroupBox("Text");
    textBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QHBoxLayout *textLayout = new QHBoxLayout(textBox);
    textLayout->setContentsMargins(5, 5, 5, 5);

    m_sizeBtn = new QPushButton("0x0");
    m_sizeBtn->setFocusPolicy(Qt::NoFocus);
    m_sizeBtn->setCursor(Qt::PointingHandCursor);
    m_sizeBtn->setToolTip("Send 'stty rows X cols Y; TERM=xterm-256color' to remote");

    textLayout->addWidget(new QLabel("Size:"));
    textLayout->addWidget(m_sizeBtn);
    textLayout->addSpacing(10);
    textLayout->addWidget(m_fontCombo);
    textLayout->addWidget(m_fontSizeSpin);
    textLayout->addSpacing(10);
    textLayout->addWidget(m_clearBtn);

    statusLayout->addWidget(textBox);
    statusLayout->addStretch();

    statusLayout->addWidget(statBox);

    mainLayout->addLayout(statusLayout, 0);

    connect(m_portCombo, &PortComboBox::popupAboutToBeShown, this, &MainWindow::refreshPorts);

    connect(m_portCombo, &QComboBox::currentTextChanged, this, &MainWindow::onPortChanged);

    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::toggleConnection);

    connect(m_clearBtn, &QPushButton::clicked, m_terminal, &VTermWidget::clearTerminal);

    connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);

    connect(m_terminal, &VTermWidget::dataReadyToSend, this, &MainWindow::writeData);

    connect(m_rtsCheck, &QCheckBox::toggled, this, &MainWindow::onRtsToggled);
    connect(m_dtrCheck, &QCheckBox::toggled, this, &MainWindow::onDtrToggled);

    m_statusTimer = new QTimer(this);
    m_statusTimer->setInterval(200);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::updateStatusLines);

    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int size){
        QFont f = m_terminal->font();
        f.setPointSize(size);
        m_terminal->setTermFont(f);
    });

    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &MainWindow::applyFont);
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::applyFont);

    connect(m_terminal, &VTermWidget::terminalSizeChanged, this, [this](int rows, int cols){
        m_sizeBtn->setText(QString("%1x%2").arg(rows).arg(cols));
    m_sizeBtn->setToolTip(QString("Send 'stty rows %1 cols %2; TERM=xterm-256color' to remote").arg(rows).arg(cols));

        m_termRows = rows;
        m_termCols = cols;

        if (m_scriptCheck->isChecked()) {
            QByteArray response = m_lua.triggerResize(rows, cols);

            if (!response.isEmpty()) {
                // If script has anything to say, we put it on the local screen
                m_terminal->writeInput(response);
            }
        }
    });

    connect(m_scriptCheck, &QCheckBox::checkStateChanged, this, [this](){
        if (m_scriptCheck->isChecked()) {
            QByteArray response = m_lua.triggerResize(m_termRows, m_termCols);

            if (!response.isEmpty()) {
                m_terminal->writeInput(response);
            }
        }

    });
    connect(m_sizeBtn, &QPushButton::clicked, this, [this](){
        if (!m_serial->isOpen()) return;

        // Composing command "stty rows <R> cols <C>; TERM=xterm-256color" + Enter
        QString cmd = QString("stty rows %1 cols %2; TERM=xterm-256color\r").arg(m_termRows).arg(m_termCols);
        m_serial->write(cmd.toUtf8());
        m_terminal->setFocus();
    });

    // --- FILE TRANSFER PROCESS (SZ / RZ) ---
    m_transferProcess = new QProcess(this);

    // 1. Redirect process output (stdout) -> Serial Port
    connect(m_transferProcess, &QProcess::readyReadStandardOutput, this, [this](){
        m_serial->write(m_transferProcess->readAllStandardOutput());
    });

    // 2. Info and Progress (stderr) -> Status Bar (LIVE)
    connect(m_transferProcess, &QProcess::readyReadStandardError, this, [this](){
	// Reading line, stripping ending (\n) and forwarding to Status Bar
        QByteArray data = m_transferProcess->readAllStandardError();
        QString msg = QString::fromUtf8(data).trimmed();

        if (!msg.isEmpty()) {
            statusBar()->showMessage(msg,5000);
        }
    });

    // 3. Cleanup (TEXT UNCHANGED)
    connect(m_transferProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus){

                resetTransferUI();
            });

    refreshPorts();
    resize(800, 600);
    loadSettings();

}

MainWindow::~MainWindow()
{
    if (m_serial->isOpen())
        m_serial->close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::onPortChanged(const QString &portName)
{
    if (portName.isEmpty()) return;

    QSettings settings("Jakub Ladman", "debuQterm");
    QString prefix = "PortSettings/" + portName;

    int savedBaud = settings.value(prefix + "_Baud", 115200).toInt();
    int baudIdx = m_baudCombo->findData(savedBaud);
    if (baudIdx != -1) {
        m_baudCombo->setCurrentIndex(baudIdx);
    }

    QString savedFrame = settings.value(prefix + "_Frame", "8N1").toString();
    int frameIdx = m_frameCombo->findData(savedFrame);
    if (frameIdx != -1) {
        m_frameCombo->setCurrentIndex(frameIdx);
    } else {
        // Fallback if something is wrong
        m_frameCombo->setCurrentIndex(m_frameCombo->findData("8N1"));
    }

    // 3. Load Flow Control (Default to false/unchecked)
    bool savedFlow = settings.value(prefix + "_Flow", false).toBool();
    m_flowCheck->setChecked(savedFlow);
}
void MainWindow::saveSettings()
{
    QSettings settings("Jakub Ladman", "debuQterm");

    // 1. Global Window Settings (Geometry = Terminal Size)
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // 2. Font Settings
    settings.setValue("fontFamily", m_fontCombo->currentFont().family());
    settings.setValue("fontSize", m_fontSizeSpin->value());

    // 3. Port Selection
    settings.setValue("lastPort", m_portCombo->currentText());

    // 4. Per-Port Settings (Baud, Frame, Flow)
    QString currentPort = m_portCombo->currentText();
    if (!currentPort.isEmpty()) {
        QString prefix = "PortSettings/" + currentPort;

        // Save Baud Rate
        settings.setValue(prefix + "_Baud", m_baudCombo->currentData().toInt());

        // Save Frame Format (e.g., "8N1", "7E1")
        settings.setValue(prefix + "_Frame", m_frameCombo->currentData().toString());

        // Save Flow Control (true/false)
        settings.setValue(prefix + "_Flow", m_flowCheck->isChecked());
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("Jakub Ladman", "debuQterm");

    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
    }
    if (settings.contains("windowState")) {
        restoreState(settings.value("windowState").toByteArray());
    }

    if (settings.contains("fontFamily")) {
        QString family = settings.value("fontFamily").toString();
        m_fontCombo->setCurrentFont(QFont(family));
    }
    if (settings.contains("fontSize")) {
        int size = settings.value("fontSize").toInt();
        m_fontSizeSpin->setValue(size);
    }

    applyFont();

    QString lastPort = settings.value("lastPort").toString();
    if (!lastPort.isEmpty()) {
        int idx = m_portCombo->findText(lastPort);
        if (idx != -1) {
            m_portCombo->setCurrentIndex(idx);
        } else {
            // Port no longer available, selecting firt one (index 0)
            if (m_portCombo->count() > 0) m_portCombo->setCurrentIndex(0);
        }
    }


    // Retrieve last Baud Rate for the port
    onPortChanged(m_portCombo->currentText());
}

void MainWindow::refreshPorts()
{
    QString currentSelection = m_portCombo->currentText();

    QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();
    QSettings settings("Jakub Ladman", "debuQterm");

    // 2. Ordering ports according to usefulness
    std::sort(infos.begin(), infos.end(), [&](const QSerialPortInfo &a, const QSerialPortInfo &b) {
        bool aSaved = settings.contains("PortSettings/" + a.portName() + "_Baud");
        bool bSaved = settings.contains("PortSettings/" + b.portName() + "_Baud");
        if (aSaved != bSaved) return aSaved > bSaved;

        auto isPriority = [](const QString &n) { return n.contains("USB") || n.contains("ACM"); };
        bool aPrio = isPriority(a.portName());
        bool bPrio = isPriority(b.portName());
        if (aPrio != bPrio) return aPrio > bPrio;

        return a.portName() < b.portName();
    });

    m_portCombo->blockSignals(true);
    m_portCombo->clear();

    for (const QSerialPortInfo &info : std::as_const(infos)) {
        QString name = info.portName();

        // --- FILTER ---
        // For "ttyS", letting go only 0, 1, 2, 3.
        // The rest (ttyS4...ttyS31) is discarded
        if (name.startsWith("ttyS")) {
            if (name != "ttyS0" && name != "ttyS1" && name != "ttyS2" && name != "ttyS3") {
                continue; // Skip
            }
        }
        // -------------------

        QString label = name;
        if (!info.description().isEmpty()) {
            label += " (" + info.description() + ")";
        }

        m_portCombo->addItem(label, name);
    }

    int idx = m_portCombo->findData(currentSelection);
    if (idx == -1) idx = m_portCombo->findText(currentSelection);

    if (idx != -1) {
        m_portCombo->setCurrentIndex(idx);
    } else if (m_portCombo->count() > 0) {
        m_portCombo->setCurrentIndex(0);
    }

    m_portCombo->blockSignals(false);
}

void MainWindow::toggleConnection()
{
    if (m_serial->isOpen()) {
        // --- DISCONNECTING ---
        m_serial->close();
        m_statusTimer->stop();

        m_connectBtn->setText("Connect");
        m_portCombo->setEnabled(true);
        m_baudCombo->setEnabled(true);
        m_flowCheck->setEnabled(true);
        m_frameCombo->setEnabled(true);

        m_rtsCheck->setEnabled(false);
        m_dtrCheck->setEnabled(false);

        setLedState(m_ctsLabel, false);
        setLedState(m_dsrLabel, false);
        setLedState(m_dcdLabel, false);
        setLedState(m_riLabel, false);

        m_terminal->writeInput("\r\n--- Disconnected ---\r\n");
    } else {
        // --- CONNECTING ---
        QString portName = m_portCombo->currentData().toString();
        if (portName.isEmpty()) return;

        m_serial->setPortName(portName);
        m_serial->setBaudRate(m_baudCombo->currentData().toInt());

        // Parse Frame Format (Data/Parity/Stop)
        QString code = m_frameCombo->currentData().toString();

        // 1. Data Bits (First character)
        if (code.at(0) == '7') {
            m_serial->setDataBits(QSerialPort::Data7);
        } else {
            m_serial->setDataBits(QSerialPort::Data8);
        }

        // 2. Parity (Second character)
        switch (code.at(1).toLatin1()) {
        case 'N': m_serial->setParity(QSerialPort::NoParity);   break;
        case 'E': m_serial->setParity(QSerialPort::EvenParity); break;
        case 'O': m_serial->setParity(QSerialPort::OddParity);  break;
        default:  m_serial->setParity(QSerialPort::NoParity);   break;
        }

        // 3. Stop Bits (Third character)
        if (code.at(2) == '2') {
            m_serial->setStopBits(QSerialPort::TwoStop);
        } else {
            m_serial->setStopBits(QSerialPort::OneStop);
        }

        if (m_flowCheck->isChecked()) {
            m_serial->setFlowControl(QSerialPort::HardwareControl);
        } else {
            m_serial->setFlowControl(QSerialPort::NoFlowControl);
        }

        if (m_serial->open(QIODevice::ReadWrite)) {
            m_connectBtn->setText("Disconnect");
            m_portCombo->setEnabled(false);
            m_baudCombo->setEnabled(false);
            m_flowCheck->setEnabled(false);
            m_frameCombo->setEnabled(false);

            m_rtsCheck->setEnabled(!m_flowCheck->isChecked());

            m_dtrCheck->setEnabled(true);

            m_serial->setRequestToSend(m_rtsCheck->isChecked());
            m_serial->setDataTerminalReady(m_dtrCheck->isChecked());

            // Start status lines polling
            m_statusTimer->start();

            QString msg = QString("\r\n--- " + portName.toUtf8() + " Connected ---\r\n");
            m_terminal->writeInput(msg.toUtf8());
            statusBar()->showMessage(msg);
            m_terminal->setFocus();
        } else {
            QMessageBox::critical(this, "Error", "Can't open port:\n" + m_serial->errorString());
        }
    }
}


void MainWindow::onRtsToggled(bool checked)
{
    if (m_serial->isOpen()) {
        m_serial->setRequestToSend(checked);
    }
}

void MainWindow::onDtrToggled(bool checked)
{
    if (m_serial->isOpen()) {
        m_serial->setDataTerminalReady(checked);
    }
}

void MainWindow::updateStatusLines()
{
    if (!m_serial->isOpen()) return;

    QSerialPort::PinoutSignals sigs = m_serial->pinoutSignals();

    // CTS (Clear To Send)
    setLedState(m_ctsLabel, sigs & QSerialPort::ClearToSendSignal);

    // DSR (Data Set Ready)
    setLedState(m_dsrLabel, sigs & QSerialPort::DataSetReadySignal);

    // DCD (Data Carrier Detect)
    setLedState(m_dcdLabel, sigs & QSerialPort::DataCarrierDetectSignal);

    // RI (Ring Indicator)
    setLedState(m_riLabel, sigs & QSerialPort::RingIndicatorSignal);
}

QLabel* MainWindow::createLedLabel(const QString &text)
{
    QLabel *lbl = new QLabel(text);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setMinimumWidth(50);
    lbl->setStyleSheet("border: 1px solid #555; border-radius: 4px; padding: 2px; color: #333; background-color: #CCC;");
    return lbl;
}

void MainWindow::setLedState(QLabel *label, bool active)
{
    if (active) {
        label->setStyleSheet("border: 1px solid #555; border-radius: 4px; padding: 2px; color: white; background-color: #00AA00; font-weight: bold;");
    } else {
        label->setStyleSheet("border: 1px solid #555; border-radius: 4px; padding: 2px; color: #555; background-color: #DDD; font-weight: normal;");
    }
}

void MainWindow::readData()
{
    QByteArray data = m_serial->readAll();

    if (data.isEmpty()) return;

    if (m_isTransferring && (m_currentProto != ProtoAscii)) {
        if (m_transferProcess->state() != QProcess::Running) {
            qWarning() << "RZ process died unexpectedly!";
            m_isTransferring = false;
            return;
        }

        if (m_transferProcess->bytesToWrite() > 128 * 1024) {
            qCritical() << "Buffer overflow protection triggered!";
            abortTransfer();
            QMessageBox::critical(this, "Transfer error",
                                  "External process isn't responding.");
            return;
        }

        m_transferProcess->write(data);
        return;
    }

    QByteArray output;

    if (m_scriptCheck->isChecked()) {
        // --- Pass through LUA ---
        output = m_lua.processRx(data);
    }
    else {
        output = data;
    }

    if (!output.isEmpty()) {
        m_terminal->writeInput(output);
        if (m_logFile.isOpen()) writeToLog(output);
    }
}

void MainWindow::writeData(const QByteArray &data)
{
    if (!m_serial->isOpen()) return;

    QByteArray dataToSend = data;

    // apply outgoing filter
    if (m_scriptCheck->isChecked()) {
        // Lua can modify or discard data
        dataToSend = m_lua.processTx(data);
    }

    if (!dataToSend.isEmpty()) {
        m_serial->write(dataToSend);
    }
}

void MainWindow::applyFont()
{
    QFont font = m_fontCombo->currentFont();

    font.setPointSize(m_fontSizeSpin->value());

    m_terminal->setTermFont(font);

    m_terminal->setFocus();
}

void MainWindow::startRecording()
{
    QSettings settings;
    QString lastDir = settings.value("logs/lastDir", QDir::homePath()).toString();

    QString fileName = QFileDialog::getSaveFileName(this, "Save log as...",
                                                    lastDir,
                                                    "Log files (*.log *.txt);;All (*.*)");

    if (fileName.isEmpty()) {
        // Cancelled by user
        m_recBtn->setChecked(false);
        return;
    }

    settings.setValue("logs/lastDir", QFileInfo(fileName).absolutePath());

    m_logFile.setFileName(fileName);
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QMessageBox::warning(this, "Error", "Can't open file for write.");
        m_recBtn->setChecked(false);
        return;
    }

    // Recording signalization, red button
//    m_recBtn->setStyleSheet("QToolButton { background-color: #ffcccc; color: red; font-weight: bold; border: 1px solid red; }");
    m_recBtn->setStyleSheet("QToolButton { background-color: #ffcccc; color: red; }");
    m_recBtn->setText("● REC");

    // Optional header
    if (m_currentLogMode == LogTimestamp) {
        m_logFile.write(QString("--- Start Recording: %1 ---\n").arg(QDateTime::currentDateTime().toString()).toUtf8());
    }
}

void MainWindow::stopRecording()
{
    if (m_logFile.isOpen()) {
        if (m_currentLogMode == LogTimestamp) {
            m_logFile.write(QString("--- Stop Recording: %1 ---\n").arg(QDateTime::currentDateTime().toString()).toUtf8());
        }
        m_logFile.close();
    }

    // Reset button
    m_recBtn->setStyleSheet(""); // Default
    m_recBtn->setText("REC");
    m_logBuffer.clear();
}

void MainWindow::writeToLog(const QByteArray &data)
{
    // 1. BINARY (Raw)
    if (m_currentLogMode == LogBin) {
        m_logFile.write(data);
        return;
    }

    // for text filtering buffering is needed
    m_logBuffer.append(data);

    int pos;
    // Searching end of line
    while ((pos = m_logBuffer.indexOf('\n')) != -1) {
        // Whole line including \n
        QByteArray line = m_logBuffer.left(pos + 1);
        m_logBuffer.remove(0, pos + 1);

        QByteArray cleanLine = stripEscapeCodes(line);

        // Write to file according to mode
        if (m_currentLogMode == LogTimestamp) {
            QString timeStr = QTime::currentTime().toString("[HH:mm:ss.zzz] ");
            m_logFile.write(timeStr.toUtf8() + cleanLine);
        }
        else {
            m_logFile.write(cleanLine);
        }
    }

    // WARNING: Data not ending with \n left in buffer waiting for next batch (needed for filter)
}

QByteArray MainWindow::stripEscapeCodes(const QByteArray &data)
{
    QString text = QString::fromUtf8(data);
    QRegularExpression::PatternOptions options = QRegularExpression::DotMatchesEverythingOption;

    // 1. OSC - ESC ] ... (BEL nebo ESC \)
    // Ex: ESC]3008;...ESC\"
    // Most of garbage, filtering out first
    static QRegularExpression oscRegex("\x1B\\].*?(\x07|\x1B\\\\)", options);
    text.remove(oscRegex);

    // 2. CSI (Color and control) - ESC [ ...
    // Range: ESC [ (anything from "0-9;?!=<>") (ends with @-~)
    // Catching ESC[0m, ESC[?2004h, ESC[1;31m etc.
    static QRegularExpression csiRegex("\x1B\\[[0-9;?!=<>]*[@-~]", options);
    text.remove(csiRegex);

    // 3. DCS and other weird sequences - ESC P ... ESC \"
    static QRegularExpression dcsRegex("\x1BP.*?(\x1B\\\\)", options);
    text.remove(dcsRegex);

    // 4. Simple ESC sequences
    // Ex: ESC 7, ESC 8, ESC M, ESC =
    static QRegularExpression simpleRegex("\x1B[@-_a-zA-Z0-9=><]", options);
    text.remove(simpleRegex);

    // 5. Remains
    text.remove('\r');   // CR
    text.remove('\x07'); // BEL

    text.remove('\x1B');

    return text.toUtf8();
}

void MainWindow::sendFile()
{
    // --- STOP LOGIC ---
    if (m_isTransferring) {
        abortTransfer();
        return;
    }

    if (!m_serial->isOpen()) {
        QMessageBox::warning(this, "Error", "Port not open.");
        return;
    }

    QSettings settings;
    QString lastDir = settings.value("files/lastDir", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this, "Select file",
                                    lastDir,
                                    "All (*.*)");
    if (fileName.isEmpty()) return;

    settings.setValue("files/lastDir", QFileInfo(fileName).absolutePath());

    // --- CLEAN TEXT BRANCH ---
    if (m_currentProto == ProtoAscii) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Error", "File can't be read.");
            return;
        }

        // Reading whole file and chopping it line by line
        QTextStream in(&file);
        QString content = in.readAll();

        m_asciiLines = content.split('\n');
        m_asciiTotalLines = m_asciiLines.count();

        m_isTransferring = true;

        m_sendBtn->setText("STOP Sending");
        m_receiveBtn->setEnabled(false);

        // Starting
        statusBar()->showMessage(QString("Sending text: %1 line(s)...").arg(m_asciiTotalLines));
        m_asciiTimer->start();
        return; // END OF METHOD, timer continues ...
    }
    // ----------------------------
    QFileInfo fi(fileName);
    QString program;
    QStringList arguments;

    switch (m_currentProto) {
    case ProtoZModem:
        program = "sz";
        // -b = binary
        // -e = escape (safe for serial)
        // -w 2048 = Window size constrained to prevent overflow
        arguments << "--zmodem" << "-b" << "-e" << "-vv" << "-w" << "2048";
        break;

    case ProtoYModem:
        program = "sz";
        arguments << "--ymodem" << "-b" << "-vv";
        break;

    case ProtoXModem:
        program = "sz";
        arguments << "--xmodem" << "-b" << "-k" << "-vv";
        break;
    default:
        return;
    }

    arguments << fi.absoluteFilePath();

    qDebug() << "Launching:" << program << arguments.join(" ");

    m_transferProcess->start(program, arguments);

    if (!m_transferProcess->waitForStarted()) {
        QMessageBox::critical(this, "Error", "Process can't be run: '" + program + "'");
        return;
    }

    m_isTransferring = true;

    m_sendBtn->setText("STOP Transfer");
    m_receiveBtn->setEnabled(false);

    QString protoName = (m_currentProto == ProtoZModem) ? "Z-Modem" :
                            (m_currentProto == ProtoYModem) ? "Y-Modem" : "X-Modem";
    statusBar()->showMessage("Sending: " + fi.fileName() + " (" + protoName + ")");
}

void MainWindow::onTransferDataFromProcess()
{
    QByteArray data = m_transferProcess->readAllStandardOutput();
    m_serial->write(data);
    // m_serial->flush();
}

void MainWindow::receiveFile()
{
    // --- STOP LOGIC ---
    if (m_isTransferring) {
        abortTransfer();
        return;
    }

    if (!m_serial->isOpen()) {
        QMessageBox::warning(this, "Error", "Port not open.");
        return;
    }

    QString program;
    QStringList arguments;
    QString workingDir;

    QSettings settings;
    QString lastDir = settings.value("files/lastDir", QDir::homePath()).toString();


    if (m_currentProto == ProtoXModem) {
        // A) X-Modem: File name needed in advance
        QString saveFileName = QFileDialog::getSaveFileName(this,
                                                            "Save incoming X-Modem file as...",
                                                            lastDir,
                                                            "All files (*.*)");

        if (saveFileName.isEmpty()) return;

        settings.setValue("files/lastDir", QFileInfo(saveFileName).absolutePath());

        QFileInfo fi(saveFileName);
        workingDir = fi.absolutePath();

        program = "rz";
        // Arguments: -b (binary), -vv (verbose for statusbar), FILE NAME
        arguments << "--xmodem" << "-b" << "-vv" << fi.fileName();
    }
    else {
        // B) Z-Modem / Y-Modem: Folder
        QString saveDir = QFileDialog::getExistingDirectory(this,
                                                            "Select folder for incoming files",
                                                            lastDir);

        if (saveDir.isEmpty()) return;

        settings.setValue("files/lastDir", QFileInfo(saveDir).absolutePath());
        workingDir = saveDir;

        if (m_currentProto == ProtoZModem) {
            program = "rz";
            // -E = rename (if exists does file.0, file.1...)
            arguments << "-b" << "-E" << "-vv";
        } else {
            program = "rz"; // --ymodem Y-Modem
            arguments << "--ymodem" << "-b" << "-vv";
        }
    }



    // --- COMMON PART ---
    m_transferProcess->setProcessChannelMode(QProcess::SeparateChannels);
    m_transferProcess->setReadChannel(QProcess::StandardOutput);
    m_transferProcess->setWorkingDirectory(workingDir);
    m_transferProcess->start(program, arguments);

    if (m_transferProcess->waitForStarted()) {
        m_isTransferring = true;

        m_receiveBtn->setText("STOP Transfer");
        m_sendBtn->setEnabled(false);

        QString protoName = (m_currentProto == ProtoZModem) ? "Z-Modem" :
                                (m_currentProto == ProtoYModem) ? "Y-Modem" : "X-Modem";

        if (m_currentProto == ProtoXModem) {
            statusBar()->showMessage(protoName + ": Waiting... (Start the transmit at the remote side!)");
        } else {
            statusBar()->showMessage(protoName + ": Waiting for data...");
        }
    } else {
        QMessageBox::critical(this, "Error", "Can't launch '" + program + "'.");
    }
}

void MainWindow::sendNextAsciiLine()
{
    // No more lines to send
    if (m_asciiLines.isEmpty()) {
        m_asciiTimer->stop();
        statusBar()->showMessage("Text transfer finished.", 5000);
        resetTransferUI();
        return;
    }

    QString line = m_asciiLines.takeFirst();

    QByteArray dataToSend = line.toUtf8() + '\r';

    m_serial->write(dataToSend);

    // Status bar update
    int remaining = m_asciiLines.count();
    int sent = m_asciiTotalLines - remaining;
    int percent = (m_asciiTotalLines > 0) ? (sent * 100 / m_asciiTotalLines) : 100;

    statusBar()->showMessage(QString("Sending text: %1% (%2/%3)").arg(percent).arg(sent).arg(m_asciiTotalLines));
}

void MainWindow::resetTransferUI()
{
    m_isTransferring = false;

    switch(m_currentProto){
    case ProtoZModem:
        m_sendBtn->setText("Send (Z-Modem)");
        m_receiveBtn->setText("Receive (Z-Modem)");
        break;
    case ProtoYModem:
        m_sendBtn->setText("Send (Y-Modem)");
        m_receiveBtn->setText("Receive (Y-Modem)");
        break;
    case ProtoXModem:
        m_sendBtn->setText("Send (X-Modem)");
        m_receiveBtn->setText("Receive (X-Modem)");
        break;
    case ProtoAscii:
        m_sendBtn->setText("Send (Text)");
        break;
    }

    m_sendBtn->setEnabled(true);
    m_receiveBtn->setEnabled(true);

    m_terminal->setEnabled(true);
    m_terminal->setFocus();
}

void MainWindow::abortTransfer()
{
    if (m_transferProcess->state() == QProcess::Running) {
        m_transferProcess->terminate();

        if (!m_transferProcess->waitForFinished(500)) {
            m_transferProcess->kill();
        }

        // Send STOP signal to peer
        // 5x Byte 0x18 (CAN - Cancel) standard abort command for X/Y/Z-Modem
        if (m_serial->isOpen()) {
            m_serial->write("\x18\x18\x18\x18\x18\x08\x08\x08\x08\x08");
        }

    }

    if (m_currentProto == ProtoAscii && m_asciiTimer->isActive()) {
        m_asciiTimer->stop();
        m_asciiLines.clear();
    }

    // Renew buttons
    resetTransferUI();

    statusBar()->showMessage("Transfer aborted by user.", 5000);
}

void MainWindow::onScriptButtonClicked()
{
    QSettings settings;
    QString lastDir = settings.value("scripts/lastDir", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(this, "Select Lua Script",
                                                    lastDir, "Lua Scripts (*.lua)");

    if (fileName.isEmpty()) {
        m_terminal->setFocus();
        return;
    }

    settings.setValue("scripts/lastDir", QFileInfo(fileName).absolutePath());

    if (m_lua.loadScript(fileName)) {
        statusBar()->showMessage("Script loaded: " + QFileInfo(fileName).fileName(), 3000);
        m_scriptCheck->setChecked(true);
        m_lua.setGlobalInt("TERM_ROWS", m_termRows);
        m_lua.setGlobalInt("TERM_COLS", m_termCols);
    } else {
        QMessageBox::critical(this, "Error", m_lua.getLastError());
    }

    if (m_serial->isOpen() && m_serial->bytesAvailable() > 0) {
        readData();
    }
}


void MainWindow::onLuaTick()
{
    qint64 realDelta = m_luaStatsTimer.restart();
    if (realDelta > 1000) realDelta = 1000;

    if (m_scriptCheck->isChecked()) {

        QByteArray dataToSend = m_lua.triggerTick((int)realDelta);

        if (!dataToSend.isEmpty()) {

            if (m_serial->isOpen()) {
                m_serial->write(dataToSend);
            }
        }
    }
}
