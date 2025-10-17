#include "HidDeviceTab.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSplitter>
#include <QDateTime>

namespace {
    constexpr int INPUT_DISPLAY_MAX_HEIGHT = 150;
    constexpr int MAX_INPUT_DISPLAY_LINES = 100;
    
    constexpr int COL_WIDTH_BYTE = 60;
    constexpr int COL_WIDTH_TRIGGER = 100;
    constexpr int COL_WIDTH_VALUE = 60;
    constexpr int COL_WIDTH_ENABLED = 70;
}

HidDeviceTab::HidDeviceTab(MidiEngine *midiEngine, QWidget *parent)
    : QWidget(parent)
    , m_midiEngine(midiEngine)
    , m_hidDevice(new HidDevice(this))
    , m_hidMapping(new HidMapping(this))
{
    setupUI();
    
    connect(m_hidDevice, &HidDevice::inputReceived, this, &HidDeviceTab::onHidInputReceived);
    connect(m_hidDevice, &HidDevice::deviceDisconnected, this, &HidDeviceTab::onDeviceDisconnected);
    connect(m_hidDevice, &HidDevice::errorOccurred, this, &HidDeviceTab::onDeviceError);
    
    connect(m_hidMapping, &HidMapping::midiMessageTriggered, [this](const MidiMessage &message) {
        if (m_midiEngine) {
            m_midiEngine->sendMidiMessage(message);
        }
    });
    
    onRefreshDevices();
}

HidDeviceTab::~HidDeviceTab()
{
}

void HidDeviceTab::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QLabel *noticeLabel = new QLabel("⚠️ <b>Under Construction</b> - HID Device mapping functionality is currently being developed.", this);
    noticeLabel->setStyleSheet("background-color: #fff3cd; border: 1px solid #ffeaa7; padding: 10px; border-radius: 5px; color: #856404;");
    noticeLabel->setWordWrap(true);
    noticeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(noticeLabel);
    
    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    
    setupDeviceControls();
    setupMappingControls();
    setupInputMonitor();
    
    splitter->addWidget(m_deviceGroup);
    splitter->addWidget(m_mappingGroup);
    splitter->addWidget(m_monitorGroup);
    
    splitter->setStretchFactor(0, 0); // Device controls - fixed size
    splitter->setStretchFactor(1, 1); // Mapping table - expandable
    splitter->setStretchFactor(2, 0); // Input monitor - compact
    
    mainLayout->addWidget(splitter);
}

void HidDeviceTab::setupDeviceControls()
{
    m_deviceGroup = new QGroupBox("HID Device Selection", this);
    QGridLayout *layout = new QGridLayout(m_deviceGroup);
    
    layout->addWidget(new QLabel("Device:"), 0, 0);
    m_deviceCombo = new QComboBox();
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &HidDeviceTab::onDeviceSelectionChanged);
    layout->addWidget(m_deviceCombo, 0, 1, 1, 2);
    
    m_refreshButton = new QPushButton("Refresh");
    connect(m_refreshButton, &QPushButton::clicked, this, &HidDeviceTab::onRefreshDevices);
    layout->addWidget(m_refreshButton, 0, 3);
    
    m_connectButton = new QPushButton("Connect");
    connect(m_connectButton, &QPushButton::clicked, this, &HidDeviceTab::onConnectDevice);
    layout->addWidget(m_connectButton, 1, 0);
    
    m_disconnectButton = new QPushButton("Disconnect");
    m_disconnectButton->setEnabled(false);
    connect(m_disconnectButton, &QPushButton::clicked, this, &HidDeviceTab::onDisconnectDevice);
    layout->addWidget(m_disconnectButton, 1, 1);
    
    m_connectionStatus = new QLabel("No device connected");
    m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
    layout->addWidget(m_connectionStatus, 1, 2, 1, 2);
}

void HidDeviceTab::setupMappingControls()
{
    m_mappingGroup = new QGroupBox("HID to MIDI Mappings", this);
    QVBoxLayout *layout = new QVBoxLayout(m_mappingGroup);
    
    m_mappingTable = new QTableWidget(0, 6, this);
    m_mappingTable->setHorizontalHeaderLabels({
        "Device", "Byte", "Trigger", "Value", "MIDI Message", "Enabled"
    });
    
    QHeaderView *header = m_mappingTable->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(0, QHeaderView::Stretch); // Device name
    header->setSectionResizeMode(1, QHeaderView::Fixed);   // Byte
    header->setSectionResizeMode(2, QHeaderView::Fixed);   // Trigger
    header->setSectionResizeMode(3, QHeaderView::Fixed);   // Value
    header->setSectionResizeMode(4, QHeaderView::Stretch); // MIDI
    header->setSectionResizeMode(5, QHeaderView::Fixed);   // Enabled
    
    m_mappingTable->setColumnWidth(1, COL_WIDTH_BYTE);
    m_mappingTable->setColumnWidth(2, COL_WIDTH_TRIGGER);
    m_mappingTable->setColumnWidth(3, COL_WIDTH_VALUE);
    m_mappingTable->setColumnWidth(5, COL_WIDTH_ENABLED);
    
    m_mappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_mappingTable, &QTableWidget::itemSelectionChanged, 
            this, &HidDeviceTab::onMappingSelectionChanged);
    
    layout->addWidget(m_mappingTable);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_addMappingButton = new QPushButton("Add Mapping");
    connect(m_addMappingButton, &QPushButton::clicked, this, &HidDeviceTab::onAddMapping);
    buttonLayout->addWidget(m_addMappingButton);
    
    m_editMappingButton = new QPushButton("Edit Mapping");
    m_editMappingButton->setEnabled(false);
    connect(m_editMappingButton, &QPushButton::clicked, this, &HidDeviceTab::onEditMapping);
    buttonLayout->addWidget(m_editMappingButton);
    
    m_removeMappingButton = new QPushButton("Remove Mapping");
    m_removeMappingButton->setEnabled(false);
    connect(m_removeMappingButton, &QPushButton::clicked, this, &HidDeviceTab::onRemoveMapping);
    buttonLayout->addWidget(m_removeMappingButton);
    
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);
}

void HidDeviceTab::setupInputMonitor()
{
    m_monitorGroup = new QGroupBox("Input Monitor", this);
    QVBoxLayout *layout = new QVBoxLayout(m_monitorGroup);
    
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    m_startMonitorButton = new QPushButton("Start Monitoring");
    connect(m_startMonitorButton, &QPushButton::clicked, this, &HidDeviceTab::onStartMonitoring);
    controlLayout->addWidget(m_startMonitorButton);
    
    m_stopMonitorButton = new QPushButton("Stop Monitoring");
    m_stopMonitorButton->setEnabled(false);
    connect(m_stopMonitorButton, &QPushButton::clicked, this, &HidDeviceTab::onStopMonitoring);
    controlLayout->addWidget(m_stopMonitorButton);
    
    m_clearDisplayButton = new QPushButton("Clear");
    connect(m_clearDisplayButton, &QPushButton::clicked, [this]() {
        m_inputDisplay->clear();
    });
    controlLayout->addWidget(m_clearDisplayButton);
    
    controlLayout->addStretch();
    layout->addLayout(controlLayout);
    
    m_inputDisplay = new QTextEdit();
    m_inputDisplay->setMaximumHeight(INPUT_DISPLAY_MAX_HEIGHT);
    m_inputDisplay->setReadOnly(true);
    m_inputDisplay->setFont(QFont("Consolas", 9));
    layout->addWidget(m_inputDisplay);
}

void HidDeviceTab::onRefreshDevices()
{
    m_availableDevices = HidDevice::enumerateDevices();
    updateDeviceList();
    emit statusMessage(QString("Found %1 HID devices").arg(m_availableDevices.size()));
}

void HidDeviceTab::onDeviceSelectionChanged(int index)
{
    m_connectButton->setEnabled(index > 0 && !m_hidDevice->isOpen());
}

void HidDeviceTab::onConnectDevice()
{
    int index = m_deviceCombo->currentIndex();
    if (index <= 0 || index > m_availableDevices.size()) {
        return;
    }
    
    const HidDeviceInfo &device = m_availableDevices[index - 1];
    
    if (m_hidDevice->openDevice(device.path)) {
        updateConnectionStatus();
        emit statusMessage(QString("Connected to HID device: %1").arg(device.name));
    } else {
        emit statusMessage(QString("Failed to connect to HID device: %1").arg(device.name));
    }
}

void HidDeviceTab::onDisconnectDevice()
{
    m_hidDevice->closeDevice();
    updateConnectionStatus();
    emit statusMessage("Disconnected from HID device");
}

void HidDeviceTab::onStartMonitoring()
{
    if (m_hidDevice->isOpen()) {
        m_hidDevice->startMonitoring();
        m_startMonitorButton->setEnabled(false);
        m_stopMonitorButton->setEnabled(true);
        emit statusMessage("Started HID input monitoring");
    }
}

void HidDeviceTab::onStopMonitoring()
{
    m_hidDevice->stopMonitoring();
    m_startMonitorButton->setEnabled(true);
    m_stopMonitorButton->setEnabled(false);
    emit statusMessage("Stopped HID input monitoring");
}

void HidDeviceTab::onAddMapping()
{
    QMessageBox::information(this, "HID Mapping", "HID mapping dialog will be implemented next.");
}

void HidDeviceTab::onEditMapping()
{
    QMessageBox::information(this, "HID Mapping", "HID mapping dialog will be implemented next.");
}

void HidDeviceTab::onRemoveMapping()
{
    int row = m_mappingTable->currentRow();
    if (row >= 0) {
        m_hidMapping->removeMapping(row);
        updateMappingTable();
    }
}

void HidDeviceTab::onMappingSelectionChanged()
{
    bool hasSelection = m_mappingTable->currentRow() >= 0;
    m_editMappingButton->setEnabled(hasSelection);
    m_removeMappingButton->setEnabled(hasSelection);
}

void HidDeviceTab::onHidInputReceived(const HidInputReport &report)
{
    updateInputDisplay(report.data);
    
    if (m_hidDevice->isOpen()) {
        m_hidMapping->processHidInput(m_hidDevice->getDeviceInfo().path, report.data);
    }
}

void HidDeviceTab::onDeviceDisconnected()
{
    updateConnectionStatus();
    emit statusMessage("HID device disconnected");
}

void HidDeviceTab::onDeviceError(const QString &error)
{
    emit statusMessage(QString("HID device error: %1").arg(error));
}

void HidDeviceTab::updateDeviceList()
{
    m_deviceCombo->clear();
    m_deviceCombo->addItem("Select HID Device...");
    
    for (const HidDeviceInfo &device : m_availableDevices) {
        QString displayName = QString("%1 (VID:%2 PID:%3)")
                              .arg(device.name)
                              .arg(device.vendorId, 4, 16, QChar('0'))
                              .arg(device.productId, 4, 16, QChar('0'));
        m_deviceCombo->addItem(displayName);
    }
}

void HidDeviceTab::updateConnectionStatus()
{
    if (m_hidDevice->isOpen()) {
        m_connectionStatus->setText(QString("Connected: %1").arg(m_hidDevice->getDeviceName()));
        m_connectionStatus->setStyleSheet("color: green; font-weight: bold;");
        m_connectButton->setEnabled(false);
        m_disconnectButton->setEnabled(true);
        m_startMonitorButton->setEnabled(true);
    } else {
        m_connectionStatus->setText("No device connected");
        m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
        m_connectButton->setEnabled(m_deviceCombo->currentIndex() > 0);
        m_disconnectButton->setEnabled(false);
        m_startMonitorButton->setEnabled(false);
        m_stopMonitorButton->setEnabled(false);
    }
}

void HidDeviceTab::updateMappingTable()
{
    m_mappingTable->setRowCount(0);
    
    QList<HidMappingEntry> mappings = m_hidMapping->getAllMappings();
    
    for (int i = 0; i < mappings.size(); ++i) {
        const HidMappingEntry &entry = mappings[i];
        
        m_mappingTable->insertRow(i);
        
        m_mappingTable->setItem(i, 0, new QTableWidgetItem(entry.deviceName));
        
        m_mappingTable->setItem(i, 1, new QTableWidgetItem(QString::number(entry.byteIndex)));
        
        QString triggerText;
        switch (entry.triggerType) {
            case HidTriggerType::ValueChange: triggerText = "Change"; break;
            case HidTriggerType::ValueEquals: triggerText = QString("= %1").arg(entry.triggerValue); break;
            case HidTriggerType::ValueGreater: triggerText = QString("> %1").arg(entry.triggerValue); break;
            case HidTriggerType::ValueLess: triggerText = QString("< %1").arg(entry.triggerValue); break;
            case HidTriggerType::ButtonPress: triggerText = "Press"; break;
            case HidTriggerType::ButtonRelease: triggerText = "Release"; break;
        }
        m_mappingTable->setItem(i, 2, new QTableWidgetItem(triggerText));
        
        m_mappingTable->setItem(i, 3, new QTableWidgetItem(QString::number(entry.triggerValue)));
        
        QString midiText;
        switch (entry.midiMessage.type) {
            case MidiMessage::NOTE_ON:
                midiText = QString("Note On Ch:%1 Note:%2 Vel:%3")
                           .arg(entry.midiMessage.channel + 1)
                           .arg(entry.midiMessage.note)
                           .arg(entry.midiMessage.velocity);
                break;
            case MidiMessage::NOTE_OFF:
                midiText = QString("Note Off Ch:%1 Note:%2 Vel:%3")
                           .arg(entry.midiMessage.channel + 1)
                           .arg(entry.midiMessage.note)
                           .arg(entry.midiMessage.velocity);
                break;
            case MidiMessage::CONTROL_CHANGE:
                midiText = QString("CC Ch:%1 CC:%2 Val:%3")
                           .arg(entry.midiMessage.channel + 1)
                           .arg(entry.midiMessage.controller)
                           .arg(entry.midiMessage.value);
                break;
        }
        m_mappingTable->setItem(i, 4, new QTableWidgetItem(midiText));
        
        m_mappingTable->setItem(i, 5, new QTableWidgetItem(entry.isEnabled ? "Yes" : "No"));
    }
}

void HidDeviceTab::updateInputDisplay(const QByteArray &inputData)
{
    const QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    const QString hexData = formatByteArray(inputData);
    
    m_inputDisplay->append(QString("[%1] %2").arg(timestamp, hexData));
    
    QTextDocument *doc = m_inputDisplay->document();
    if (doc->blockCount() > MAX_INPUT_DISPLAY_LINES) {
        QTextCursor cursor = m_inputDisplay->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, doc->blockCount() - MAX_INPUT_DISPLAY_LINES);
        cursor.removeSelectedText();
    }
    
    m_inputDisplay->moveCursor(QTextCursor::End);
}

QString HidDeviceTab::formatByteArray(const QByteArray &byteData) const
{
    QStringList hexBytes;
    for (int i = 0; i < byteData.size(); ++i) {
        hexBytes << QString("%1").arg(static_cast<unsigned char>(byteData[i]), 2, 16, QChar('0')).toUpper();
    }
    return QString("Size:%1 Data:[%2]").arg(byteData.size()).arg(hexBytes.join(" "));
}

void HidDeviceTab::saveSettings()
{
}

void HidDeviceTab::loadSettings()
{
}