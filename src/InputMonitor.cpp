#include "InputMonitor.h"
#include "KeyUtils.h"
#include <QDateTime>
#include <QScrollBar>
#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <windows.h>

namespace {
    constexpr int CONSOLE_FONT_SIZE = 9;
    constexpr int STATUS_MESSAGE_TIMEOUT_MS = 3000;
    const QString STATUS_READY_STYLE = "font-weight: bold; color: green;";
    const QString STATUS_PAUSED_STYLE = "font-weight: bold; color: orange;";
    const QString STATUS_DISABLED_STYLE = "font-weight: bold; color: red;";
}

InputMonitor::InputMonitor(QWidget *parent)
    : QWidget(parent)
    , m_layout(nullptr)
    , m_console(nullptr)
    , m_clearButton(nullptr)
    , m_ignoreRepeatsCheckBox(nullptr)
    , m_statusLabel(nullptr)
    , m_ignoreRepeats(false)
    , m_loggingEnabled(true)
    , m_eventCount(0)
{
    setupUI();
}

InputMonitor::~InputMonitor()
{
}

void InputMonitor::setupUI()
{
    m_layout = new QVBoxLayout(this);
    
    m_statusLabel = new QLabel("Input Monitor - Ready", this);
    m_statusLabel->setStyleSheet(STATUS_READY_STYLE);
    m_layout->addWidget(m_statusLabel);
    
    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    m_console->setFont(QFont("Consolas", CONSOLE_FONT_SIZE));
    m_console->setPlaceholderText("Key events will appear here when this tab is active and the application is focused...");
    m_layout->addWidget(m_console);
    
    QWidget *controlPanel = new QWidget(this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlPanel);
    
    m_clearButton = new QPushButton("Clear Console", controlPanel);
    connect(m_clearButton, &QPushButton::clicked, this, &InputMonitor::clearConsole);
    controlLayout->addWidget(m_clearButton);
    
    m_ignoreRepeatsCheckBox = new QCheckBox("Ignore Repeated Keys", controlPanel);
    m_ignoreRepeatsCheckBox->setToolTip("Hide repeat key events from this monitor view only (does not affect MIDI output)");
    connect(m_ignoreRepeatsCheckBox, &QCheckBox::toggled, this, &InputMonitor::onIgnoreRepeatsToggled);
    controlLayout->addWidget(m_ignoreRepeatsCheckBox);
    
    controlLayout->addStretch();
    
    QLabel *eventCountLabel = new QLabel("Events: 0", controlPanel);
    eventCountLabel->setObjectName("eventCountLabel");
    controlLayout->addWidget(eventCountLabel);
    
    m_layout->addWidget(controlPanel);
    
    setLayout(m_layout);
}

void InputMonitor::logKeyEvent(int vkCode, bool isKeyDown, bool isRepeat)
{
    if (!shouldLogEvents()) {
        return;
    }
    
    if (m_ignoreRepeats && isRepeat) {
        return;
    }
    
    QString eventText = formatKeyEvent(vkCode, isKeyDown, isRepeat);
    m_console->append(eventText);
    
    QScrollBar *scrollBar = m_console->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
    
    m_eventCount++;
    QLabel *eventCountLabel = findChild<QLabel*>("eventCountLabel");
    if (eventCountLabel) {
        eventCountLabel->setText(QString("Events: %1").arg(m_eventCount));
    }
}

QString InputMonitor::formatKeyEvent(int vkCode, bool isKeyDown, bool isRepeat)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString keyName = getKeyName(vkCode);
    QString eventType = isKeyDown ? "DOWN" : "UP";
    QString repeatFlag = isRepeat ? " [REPEAT]" : "";
    
    return QString("[%1] VK_%2 (0x%3) %4%5 - %6")
           .arg(timestamp)
           .arg(QString::number(vkCode).rightJustified(3, '0'))
           .arg(QString::number(vkCode, 16).toUpper().rightJustified(2, '0'))
           .arg(eventType)
           .arg(repeatFlag)
           .arg(keyName);
}

QString InputMonitor::getKeyName(int vkCode) const
{
    return KeyUtils::getKeyName(vkCode);
}

void InputMonitor::clearConsole()
{
    m_console->clear();
    m_eventCount = 0;
    QLabel *eventCountLabel = findChild<QLabel*>("eventCountLabel");
    if (eventCountLabel) {
        eventCountLabel->setText("Events: 0");
    }
}

void InputMonitor::setLoggingEnabled(bool enabled)
{
    m_loggingEnabled = enabled;
    
    if (enabled) {
        m_statusLabel->setText("Input Monitor - Active");
        m_statusLabel->setStyleSheet(STATUS_READY_STYLE);
    } else {
        m_statusLabel->setText("Input Monitor - Paused");
        m_statusLabel->setStyleSheet(STATUS_PAUSED_STYLE);
    }
}

void InputMonitor::setIgnoreRepeats(bool ignore)
{
    m_ignoreRepeats = ignore;
    m_ignoreRepeatsCheckBox->setChecked(ignore);
}

bool InputMonitor::shouldLogEvents() const
{
    if (!m_loggingEnabled || !isVisible()) {
        return false;
    }
    
    QWidget *topLevel = window();
    return topLevel && topLevel->isActiveWindow();
}

void InputMonitor::onIgnoreRepeatsToggled(bool checked)
{
    m_ignoreRepeats = checked;
}