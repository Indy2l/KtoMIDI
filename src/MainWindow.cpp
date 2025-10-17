#include "MainWindow.h"
#include "KeyUtils.h"
#include <QApplication>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSplitter>
#include <QStandardPaths>
#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QStatusBar>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QSettings>
#include <windows.h>

namespace {
    constexpr int STATUS_MESSAGE_TIMEOUT_MS = 3000;
    constexpr int TRAY_MESSAGE_TIMEOUT_MS = 5000;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_keyHook(nullptr)
    , m_midiEngine(nullptr)
    , m_keyMapping(nullptr)
    , m_inputMonitor(nullptr)
    , m_trayIcon(nullptr)
    , m_currentEditingVkCode(-1)
    , m_isEditingMapping(false)
    , m_waitingForKeyPress(false)
    , m_currentMappingDialog(nullptr)
    , m_shouldAutoConnect(false)
{
    setupUI();
    setupSystemTray();
    
    m_keyHook = new KeyHook(this);
    m_midiEngine = new MidiEngine(this);
    m_keyMapping = new KeyMapping(this);
    
    connect(m_keyHook, &KeyHook::keyPressed, this, &MainWindow::onKeyPressed);
    connect(m_midiEngine, &MidiEngine::portOpened, this, &MainWindow::onMidiPortOpened);
    connect(m_midiEngine, &MidiEngine::portClosed, this, &MainWindow::onMidiPortClosed);
    connect(m_midiEngine, &MidiEngine::errorOccurred, this, &MainWindow::onMidiError);
    connect(m_keyMapping, &KeyMapping::midiMessageTriggered, this, &MainWindow::onMidiMessageTriggered);
    connect(m_keyMapping, &KeyMapping::mappingAdded, this, [this](const KeyMappingEntry &) { updateSuppressedKeys(); });
    connect(m_keyMapping, &KeyMapping::mappingRemoved, this, [this](int) { updateSuppressedKeys(); });
    connect(m_keyMapping, &KeyMapping::mappingUpdated, this, [this](const KeyMappingEntry &) { updateSuppressedKeys(); });
    
    if (!m_keyHook->installHook()) {
        QMessageBox::warning(this, "Keyboard Hook", 
            "Failed to install keyboard hook. Key capture may not work properly.");
    }
    
    loadSettings();
    refreshMidiPorts();
}

MainWindow::~MainWindow()
{
    saveSettings();
    
    if (m_keyHook) {
        m_keyHook->uninstallHook();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("KtoMIDI - Keyboard and HID to MIDI Converter");
    
    setWindowIcon(getApplicationIcon());
    
    resize(800, 600);
    setMinimumSize(600, 400);
    
    statusBar()->showMessage("Ready");
    
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    setupConfigurationTab();
    setupInputMonitorTab();
    
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
}

void MainWindow::setupConfigurationTab()
{
    m_configTab = new QWidget();
    m_tabWidget->addTab(m_configTab, "Configuration");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(m_configTab);
    
    setupMidiControls();
    setupSystemControls();
    setupMappingTable();
    
    mainLayout->addWidget(m_midiGroup);
    mainLayout->addWidget(m_systemGroup);
    mainLayout->addWidget(m_mappingGroup);
}

void MainWindow::setupInputMonitorTab()
{
    m_inputMonitor = new InputMonitor();
    m_inputMonitorTab = m_inputMonitor;
    m_tabWidget->addTab(m_inputMonitorTab, "Input Monitor");
    
    m_hidDeviceTab = new HidDeviceTab(m_midiEngine, this);
    connect(m_hidDeviceTab, &HidDeviceTab::statusMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, STATUS_MESSAGE_TIMEOUT_MS);
    });
    m_tabWidget->addTab(m_hidDeviceTab, "HID Devices");
}

void MainWindow::setupMidiControls()
{
    m_midiGroup = new QGroupBox("MIDI Output", m_configTab);
    QVBoxLayout *midiVerticalLayout = new QVBoxLayout(m_midiGroup);
    
    QHBoxLayout *midiLayout = new QHBoxLayout();
    
    m_midiPortLabel = new QLabel("MIDI Port:");
    midiLayout->addWidget(m_midiPortLabel);
    
    m_midiPortCombo = new QComboBox();
    m_midiPortCombo->setMinimumWidth(200);
    connect(m_midiPortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMidiPortChanged);
    midiLayout->addWidget(m_midiPortCombo);
    
    m_refreshPortsButton = new QPushButton("Refresh");
    connect(m_refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshMidiPorts);
    midiLayout->addWidget(m_refreshPortsButton);
    
    midiLayout->addStretch();
    
    m_midiStatusLabel = new QLabel("No port selected");
    m_midiStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    midiLayout->addWidget(m_midiStatusLabel);
    
    midiVerticalLayout->addLayout(midiLayout);
    
    m_autoConnectCheck = new QCheckBox("Auto-connect to last port on startup");
    m_autoConnectCheck->setChecked(true); // Default to enabled
    midiVerticalLayout->addWidget(m_autoConnectCheck);
}

void MainWindow::setupSystemControls()
{
    m_systemGroup = new QGroupBox("System Settings", m_configTab);
    QVBoxLayout *systemLayout = new QVBoxLayout(m_systemGroup);
    
    m_autoStartCheck = new QCheckBox("Start with Windows");
    m_autoStartCheck->setChecked(isAutoStartEnabled());
    m_autoStartCheck->setToolTip("Automatically start KtoMIDI when Windows starts");
    connect(m_autoStartCheck, &QCheckBox::toggled, this, &MainWindow::setAutoStartEnabled);
    systemLayout->addWidget(m_autoStartCheck);
}

void MainWindow::setupMappingTable()
{
    m_mappingGroup = new QGroupBox("Key Mappings", m_configTab);
    QVBoxLayout *mappingLayout = new QVBoxLayout(m_mappingGroup);
    
    m_mappingTable = new QTableWidget(0, 4);
    QStringList headers = {"Key", "VK Code", "KeyDown", "KeyUp"};
    m_mappingTable->setHorizontalHeaderLabels(headers);
    
    m_mappingTable->horizontalHeader()->setStretchLastSection(false);
    m_mappingTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    
    m_mappingTable->setColumnWidth(0, 150);
    m_mappingTable->setColumnWidth(1, 80);
    m_mappingTable->setColumnWidth(2, 80);
    m_mappingTable->setColumnWidth(3, 80);
    
    m_mappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mappingTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    connect(m_mappingTable, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onMappingTableSelectionChanged);
    
    mappingLayout->addWidget(m_mappingTable);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    m_addMappingButton = new QPushButton("Add Mapping");
    connect(m_addMappingButton, &QPushButton::clicked, this, &MainWindow::addKeyMapping);
    buttonLayout->addWidget(m_addMappingButton);
    
    m_removeMappingButton = new QPushButton("Remove Mapping");
    m_removeMappingButton->setEnabled(false);
    connect(m_removeMappingButton, &QPushButton::clicked, this, &MainWindow::removeKeyMapping);
    buttonLayout->addWidget(m_removeMappingButton);
    
    m_editMappingButton = new QPushButton("Edit Mapping");
    m_editMappingButton->setEnabled(false);
    connect(m_editMappingButton, &QPushButton::clicked, this, &MainWindow::editKeyMapping);
    buttonLayout->addWidget(m_editMappingButton);
    
    buttonLayout->addStretch();
    
    mappingLayout->addLayout(buttonLayout);
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray is not available on this system";
        return;
    }
    
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(getApplicationIcon(QSize(16, 16)));
    m_trayIcon->setToolTip("KtoMIDI - Keyboard to MIDI Converter");
    
    m_trayMenu = new QMenu(this);
    
    m_showAction = new QAction("Show KtoMIDI", this);
    connect(m_showAction, &QAction::triggered, this, &MainWindow::showMainWindow);
    m_trayMenu->addAction(m_showAction);
    
    m_trayMenu->addSeparator();
    
    m_quitAction = new QAction("Quit", this);
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::quitApplication);
    m_trayMenu->addAction(m_quitAction);
    
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    
    m_trayIcon->show();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_trayIcon && m_trayIcon->isVisible()) {
            hide();
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::onMappingDialogKeyDetectionRequested()
{
    m_waitingForKeyPress = true;
}

void MainWindow::onKeyPressed(int vkCode, bool isKeyDown, bool isRepeat)
{
    if (m_inputMonitor) {
        m_inputMonitor->logKeyEvent(vkCode, isKeyDown, isRepeat);
    }
    
    if (m_waitingForKeyPress && m_currentMappingDialog && isKeyDown && !isRepeat) {
        m_waitingForKeyPress = false;
        m_currentMappingDialog->setDetectedVkCode(vkCode);
        return;
    }
    
    m_keyMapping->processKeyEvent(vkCode, isKeyDown, isRepeat);
}

void MainWindow::onMidiMessageTriggered(const MidiMessage &message, int vkCode, bool isKeyDown)
{
    Q_UNUSED(vkCode)
    Q_UNUSED(isKeyDown)
    
    if (m_midiEngine && m_midiEngine->isPortOpen()) {
        m_midiEngine->sendMidiMessage(message);
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showMainWindow();
    }
}

void MainWindow::showMainWindow()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::quitApplication()
{
    QApplication::quit();
}

void MainWindow::onTabChanged(int index)
{
    if (m_inputMonitor) {
        bool isInputMonitorTab = (m_tabWidget->widget(index) == m_inputMonitor);
        m_inputMonitor->setLoggingEnabled(isInputMonitorTab);
    }
}

void MainWindow::refreshMidiPorts()
{
    if (!m_midiEngine) return;
    
    QStringList ports = m_midiEngine->getAvailablePorts();
    
    m_midiPortCombo->clear();
    m_midiPortCombo->addItem("Select MIDI Port...");
    m_midiPortCombo->addItems(ports);
    
    updateMidiPortStatus();
    
    if (m_shouldAutoConnect && !m_pendingAutoConnectPort.isEmpty()) {
        int index = m_midiPortCombo->findText(m_pendingAutoConnectPort);
        if (index > 0) {
            m_midiPortCombo->setCurrentIndex(index);
            onMidiPortChanged(index);
        }
        
        m_shouldAutoConnect = false;
        m_pendingAutoConnectPort.clear();
    }
}

void MainWindow::onMidiPortChanged(int index)
{
    if (index <= 0) {
        m_midiEngine->closePort();
        return;
    }
    
    if (m_midiEngine->openPort(index - 1)) {
        updateMidiPortStatus();
    }
}

void MainWindow::onMidiPortOpened(const QString &portName)
{
    m_midiStatusLabel->setText(QString("Connected: %1").arg(portName));
    m_midiStatusLabel->setStyleSheet("color: green; font-weight: bold;");
    statusBar()->showMessage(QString("MIDI: Connected to %1").arg(portName), STATUS_MESSAGE_TIMEOUT_MS);
}

void MainWindow::onMidiPortClosed()
{
    m_midiStatusLabel->setText("Not connected");
    m_midiStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    statusBar()->showMessage("MIDI: Disconnected", STATUS_MESSAGE_TIMEOUT_MS);
}

void MainWindow::onMidiError(const QString &error)
{
    showMessage("MIDI Error", error, QSystemTrayIcon::Critical);
}

void MainWindow::updateMidiPortStatus()
{
    if (m_midiEngine->isPortOpen()) {
        onMidiPortOpened(m_midiEngine->getCurrentPortName());
    } else {
        onMidiPortClosed();
    }
}

QString MainWindow::getKeyName(int vkCode) const
{
    return KeyUtils::getKeyName(vkCode);
}

void MainWindow::showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, icon, TRAY_MESSAGE_TIMEOUT_MS);
    } else {
        QMessageBox::information(this, title, message);
    }
}

void MainWindow::loadSettings()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString settingsFile = appDataPath + "/settings.json";
    
    QFile file(settingsFile);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return;
    }
    
    QJsonObject obj = doc.object();
    
    restoreGeometry(QByteArray::fromBase64(obj["geometry"].toString().toUtf8()));
    restoreState(QByteArray::fromBase64(obj["windowState"].toString().toUtf8()));
    
    bool autoConnect = obj["autoConnectMidi"].toBool(true);
    m_autoConnectCheck->setChecked(autoConnect);
    
    m_shouldAutoConnect = autoConnect;
    m_pendingAutoConnectPort = obj["midiPort"].toString();
    
    QString mappingsFile = appDataPath + "/mappings.json";
    if (QFile::exists(mappingsFile)) {
        m_keyMapping->loadFromFile(mappingsFile);
        updateMappingTable();
        updateSuppressedKeys();
    }
}

void MainWindow::saveSettings()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    QJsonObject obj;
    obj["geometry"] = QString(saveGeometry().toBase64());
    obj["windowState"] = QString(saveState().toBase64());
    obj["autoConnectMidi"] = m_autoConnectCheck->isChecked();
    
    if (m_midiEngine && m_midiEngine->isPortOpen()) {
        obj["midiPort"] = m_midiEngine->getCurrentPortName();
    }
    
    QString settingsFile = appDataPath + "/settings.json";
    QFile file(settingsFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.close();
    }
    
    QString mappingsFile = appDataPath + "/mappings.json";
    m_keyMapping->saveToFile(mappingsFile);
}

void MainWindow::addKeyMapping()
{
    MappingDialog *dialog = new MappingDialog(this);
    connect(dialog, &MappingDialog::keyDetectionRequested, this, &MainWindow::onMappingDialogKeyDetectionRequested);
    
    m_currentMappingDialog = dialog;
    
    if (dialog->exec() == QDialog::Accepted) {
        KeyMappingEntry entry = dialog->getMappingEntry();
        if (entry.vkCode > 0) {
            if (m_keyMapping->hasMapping(entry.vkCode)) {
                QMessageBox::warning(this, "Key Mapping", 
                    QString("A mapping already exists for %1 (VK_%2).\nDo you want to replace it?")
                    .arg(entry.keyName).arg(entry.vkCode));
                m_keyMapping->updateMapping(entry);
            } else {
                m_keyMapping->addMapping(entry);
            }
            updateMappingTable();
        }
    }
    
    m_currentMappingDialog = nullptr;
    dialog->deleteLater();
}

void MainWindow::removeKeyMapping()
{
    int currentRow = m_mappingTable->currentRow();
    if (currentRow < 0) return;
    
    QTableWidgetItem *vkCodeItem = m_mappingTable->item(currentRow, 1);
    if (!vkCodeItem) return;
    
    int vkCode = vkCodeItem->text().toInt();
    m_keyMapping->removeMapping(vkCode);
    updateMappingTable();
    
    m_removeMappingButton->setEnabled(false);
    m_editMappingButton->setEnabled(false);
}

void MainWindow::editKeyMapping()
{
    int currentRow = m_mappingTable->currentRow();
    if (currentRow < 0) return;
    
    QTableWidgetItem *vkCodeItem = m_mappingTable->item(currentRow, 1);
    if (!vkCodeItem) return;
    
    int originalVkCode = vkCodeItem->text().toInt();
    if (m_keyMapping->hasMapping(originalVkCode)) {
        KeyMappingEntry entry = m_keyMapping->getMapping(originalVkCode);
        
        MappingDialog *dialog = new MappingDialog(entry, this);
        connect(dialog, &MappingDialog::keyDetectionRequested, this, &MainWindow::onMappingDialogKeyDetectionRequested);
        
        m_currentMappingDialog = dialog;
        
        if (dialog->exec() == QDialog::Accepted) {
            KeyMappingEntry updatedEntry = dialog->getMappingEntry();
            if (updatedEntry.vkCode > 0) {
                if (updatedEntry.vkCode != originalVkCode && m_keyMapping->hasMapping(updatedEntry.vkCode)) {
                    QMessageBox::StandardButton reply = QMessageBox::question(this, "Key Mapping", 
                        QString("A mapping already exists for %1 (VK_%2).\nDo you want to replace it?")
                        .arg(updatedEntry.keyName).arg(updatedEntry.vkCode),
                        QMessageBox::Yes | QMessageBox::No);
                    
                    if (reply != QMessageBox::Yes) {
                        m_currentMappingDialog = nullptr;
                        dialog->deleteLater();
                        return;
                    }
                }
                
                m_keyMapping->replaceMapping(originalVkCode, updatedEntry);
                updateMappingTable();
            }
        }
        
        m_currentMappingDialog = nullptr;
        dialog->deleteLater();
    }
}

void MainWindow::onMappingTableSelectionChanged()
{
    bool hasSelection = m_mappingTable->currentRow() >= 0;
    m_removeMappingButton->setEnabled(hasSelection);
    m_editMappingButton->setEnabled(hasSelection);
}

void MainWindow::updateMappingTable()
{
    m_mappingTable->setRowCount(0);
    
    QList<KeyMappingEntry> mappings = m_keyMapping->getAllMappings();
    
    for (const KeyMappingEntry &entry : mappings) {
        int row = m_mappingTable->rowCount();
        m_mappingTable->insertRow(row);
        
        m_mappingTable->setItem(row, 0, new QTableWidgetItem(entry.keyName));
        m_mappingTable->setItem(row, 1, new QTableWidgetItem(QString::number(entry.vkCode)));
        m_mappingTable->setItem(row, 2, new QTableWidgetItem(entry.enableKeyDown ? "Yes" : "No"));
        m_mappingTable->setItem(row, 3, new QTableWidgetItem(entry.enableKeyUp ? "Yes" : "No"));
    }
}

void MainWindow::updateSuppressedKeys()
{
    QSet<int> suppressedKeys;
    
    QList<KeyMappingEntry> mappings = m_keyMapping->getAllMappings();
    for (const KeyMappingEntry &entry : mappings) {
        if (entry.suppressRepeats) {
            suppressedKeys.insert(entry.vkCode);
        }
    }
    
    if (m_keyHook) {
        m_keyHook->setSuppressedRepeatKeys(suppressedKeys);
    }
}

QIcon MainWindow::getApplicationIcon() const
{
    static QIcon cachedIcon;
    static bool iconLoaded = false;
    
    if (!iconLoaded) {
        QIcon icon(":/icons/KtoMIDI.ico");
        if (!icon.isNull() && !icon.availableSizes().isEmpty()) {
            cachedIcon = icon;
        } else {
            icon = QIcon(":/images/app-icon.ico");
            if (!icon.isNull() && !icon.availableSizes().isEmpty()) {
                cachedIcon = icon;
            } else {
                QPixmap pixmap(32, 32);
                pixmap.fill(QColor(64, 128, 255));
                cachedIcon = QIcon(pixmap);
            }
        }
        iconLoaded = true;
    }
    
    return cachedIcon;
}

QIcon MainWindow::getApplicationIcon(const QSize &size) const
{
    QIcon baseIcon = getApplicationIcon();
    if (baseIcon.isNull()) {
        return baseIcon;
    }
    
    QPixmap pixmap = baseIcon.pixmap(size);
    return QIcon(pixmap);
}

void MainWindow::setAutoStartEnabled(bool enabled)
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                       QSettings::NativeFormat);
    
    if (enabled) {
        QString appPath = QCoreApplication::applicationFilePath();
        appPath = QString("\"%1\" --minimized").arg(QDir::toNativeSeparators(appPath));
        settings.setValue("KtoMIDI", appPath);
    } else {
        settings.remove("KtoMIDI");
    }
    
    if (m_autoStartCheck && m_autoStartCheck->isChecked() != enabled) {
        m_autoStartCheck->setChecked(enabled);
    }
}

bool MainWindow::isAutoStartEnabled() const
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                       QSettings::NativeFormat);
    return settings.contains("KtoMIDI");
}