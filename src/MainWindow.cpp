#include "MainWindow.h"
#include "KeyUtils.h"
#include <QMouseEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QtGlobal>
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
#include <QFile>
#include <QSettings>
#include <windows.h>
#if __has_include("version.h")
#include "version.h"
#else
#define KTOMIDI_VERSION_STRING "0.0.0"
#define KTOMIDI_APP_NAME "KtoMIDI"
#define KTOMIDI_APP_DESCRIPTION "Keycode to MIDI converter"
#define KTOMIDI_COMPANY_NAME "KtoMIDI Project"
#endif

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
    qApp->installEventFilter(this);
    
    m_keyHook = new KeyHook(this);
    m_midiEngine = new MidiEngine(this);
    m_keyMapping = new KeyMapping(this);
    
    connect(m_keyHook, &KeyHook::keyPressed, this, &MainWindow::onKeyPressed);
    connect(m_midiEngine, &MidiEngine::portOpened, this, &MainWindow::onMidiPortOpened);
    connect(m_midiEngine, &MidiEngine::portClosed, this, &MainWindow::onMidiPortClosed);
    connect(m_midiEngine, &MidiEngine::errorOccurred, this, &MainWindow::onMidiError);
    connect(m_keyMapping, &KeyMapping::midiMessageTriggered, this, &MainWindow::onMidiMessageTriggered);
    connect(m_keyMapping, &KeyMapping::mappingAdded, this, [this](const KeyMappingEntry &) { 
        updateSuppressedKeys(); 
        saveSettings();
    });
    connect(m_keyMapping, &KeyMapping::mappingRemoved, this, [this](int) { 
        updateSuppressedKeys(); 
        saveSettings();
    });
    connect(m_keyMapping, &KeyMapping::mappingUpdated, this, [this](const KeyMappingEntry &) { 
        updateSuppressedKeys(); 
        saveSettings();
    });
    
    if (!m_keyHook->installHook()) {
        QMessageBox::warning(this, "Keyboard Hook", 
            "Failed to install keyboard hook. Key capture may not work properly.");
    }
    
    loadSettings();
    updateAutoStartPath();
    refreshMidiPorts();
}

MainWindow::~MainWindow()
{
    saveSettings();
    
    if (m_keyHook) {
        m_keyHook->uninstallHook();
    }
    qApp->removeEventFilter(this);
}

void MainWindow::setupUI()
{
    setWindowTitle("KtoMIDI");
    
    setWindowIcon(getApplicationIcon());
    
    resize(800, 600);
    setMinimumSize(600, 400);
    
    statusBar()->showMessage("Ready");
    
    m_tabWidget = new QTabWidget(this);
    
    QLabel *versionLabel = new QLabel(QString("v%1").arg(KTOMIDI_VERSION_STRING));
    versionLabel->setStyleSheet("color: gray; font-size: 10pt; padding-right: 8px;");
    m_tabWidget->setCornerWidget(versionLabel, Qt::TopRightCorner);
    
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
    m_autoConnectCheck->setChecked(true);
    connect(m_autoConnectCheck, &QCheckBox::toggled, this, &MainWindow::saveSettings);
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

    m_mappingTable->viewport()->installEventFilter(this);
    
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
    m_trayIcon->setToolTip("KtoMIDI");
    
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
            if (m_midiPortCombo->currentIndex() != index) {
                m_midiPortCombo->setCurrentIndex(index);
            } else {
                onMidiPortChanged(index);
            }
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
    saveSettings();
}

void MainWindow::onMidiPortClosed()
{
    m_midiStatusLabel->setText("Not connected");
    m_midiStatusLabel->setStyleSheet("color: red; font-weight: bold;");
    statusBar()->showMessage("MIDI: Disconnected", STATUS_MESSAGE_TIMEOUT_MS);
    saveSettings();
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
        m_autoStartCheck->blockSignals(true);
        m_autoStartCheck->setChecked(isAutoStartEnabled());
        m_autoStartCheck->blockSignals(false);
        return;
    }
    
    QByteArray jsonData = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse settings JSON:" << parseError.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qWarning() << "Settings JSON is not an object";
        return;
    }
    
    QJsonObject obj = doc.object();
    
    restoreGeometry(QByteArray::fromBase64(obj["geometry"].toString().toUtf8()));
    restoreState(QByteArray::fromBase64(obj["windowState"].toString().toUtf8()));
    
    m_autoConnectCheck->blockSignals(true);
    m_autoStartCheck->blockSignals(true);
    
    bool autoConnect = obj["autoConnectMidi"].toBool(true);
    m_autoConnectCheck->setChecked(autoConnect);
    
    m_shouldAutoConnect = autoConnect;
    m_pendingAutoConnectPort = obj["midiPort"].toString();
    
    if (obj.contains("autoStart")) {
        bool autoStart = obj["autoStart"].toBool(false);
        bool registryState = isAutoStartEnabled();
        
        if (autoStart != registryState) {
            qDebug() << "Settings mismatch: JSON says autoStart=" << autoStart 
                     << "but registry says" << registryState << ". Using registry state.";
            m_autoStartCheck->setChecked(registryState);
        } else {
            m_autoStartCheck->setChecked(autoStart);
        }
    } else {
        m_autoStartCheck->setChecked(isAutoStartEnabled());
    }
    
    m_autoConnectCheck->blockSignals(false);
    m_autoStartCheck->blockSignals(false);
    
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
    obj["autoStart"] = m_autoStartCheck->isChecked();
    
    if (m_midiEngine && m_midiEngine->isPortOpen()) {
        obj["midiPort"] = m_midiEngine->getCurrentPortName();
    }
    
    QString settingsFile = appDataPath + "/settings.json";
    QFile file(settingsFile);
    if (file.open(QIODevice::WriteOnly)) {
        qint64 bytesWritten = file.write(QJsonDocument(obj).toJson());
        file.close();
        if (bytesWritten <= 0) {
            qWarning() << "Failed to write settings to" << settingsFile;
        }
    } else {
        qWarning() << "Failed to open settings file for writing:" << settingsFile;
    }
    
    QString mappingsFile = appDataPath + "/mappings.json";
    m_keyMapping->saveToFile(mappingsFile);
}

void MainWindow::addKeyMapping()
{
    QPointer<MappingDialog> dialog = new MappingDialog(this);
    connect(dialog, &MappingDialog::keyDetectionRequested, this, &MainWindow::onMappingDialogKeyDetectionRequested);
    
    m_currentMappingDialog = dialog;
    
    if (dialog->exec() == QDialog::Accepted) {
        KeyMappingEntry entry = dialog->getMappingEntry();
        if (entry.vkCode > 0) {
            bool mappingChanged = false;
            if (m_keyMapping->hasMapping(entry.vkCode)) {
                QMessageBox::StandardButton reply = QMessageBox::question(
                    this,
                    "Key Mapping",
                    QString("A mapping already exists for %1 (VK_%2).\nDo you want to replace it?")
                        .arg(entry.keyName)
                        .arg(entry.vkCode),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    m_keyMapping->updateMapping(entry);
                    mappingChanged = true;
                }
            } else {
                m_keyMapping->addMapping(entry);
                mappingChanged = true;
            }
            if (mappingChanged) {
                updateMappingTable();
            }
        }
    }
    
    m_currentMappingDialog = nullptr;
    if (dialog) {
        dialog->deleteLater();
    }
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
        
        QPointer<MappingDialog> dialog = new MappingDialog(entry, this);
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
                        if (dialog) {
                            dialog->deleteLater();
                        }
                        return;
                    }
                }
                
                m_keyMapping->replaceMapping(originalVkCode, updatedEntry);
                updateMappingTable();
            }
        }
        
        m_currentMappingDialog = nullptr;
        if (dialog) {
            dialog->deleteLater();
        }
    }
}

void MainWindow::onMappingTableSelectionChanged()
{
    bool hasSelection = m_mappingTable->currentRow() >= 0;
    m_removeMappingButton->setEnabled(hasSelection);
    m_editMappingButton->setEnabled(hasSelection);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (m_mappingTable && watched == m_mappingTable->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        QModelIndex idx = m_mappingTable->indexAt(mouseEvent->pos());
        if (!idx.isValid()) {
            m_mappingTable->clearSelection();
            m_mappingTable->setCurrentCell(-1, -1);
            m_mappingTable->clearFocus();
            onMappingTableSelectionChanged();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    QPoint globalPos;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    globalPos = mouseEvent->globalPosition().toPoint();
#else
    globalPos = mouseEvent->globalPos();
#endif
        QWidget *clickedWidget = QApplication::widgetAt(globalPos);

        bool clickedInsideMappingTable = false;
        bool clickedInsideMappingDialog = false;

        if (clickedWidget) {
            if (m_mappingTable && (m_mappingTable == clickedWidget || m_mappingTable->isAncestorOf(clickedWidget))) {
                clickedInsideMappingTable = true;
            }
            if (m_currentMappingDialog && (m_currentMappingDialog == clickedWidget || m_currentMappingDialog->isAncestorOf(clickedWidget))) {
                clickedInsideMappingDialog = true;
            }
        }

        if (!clickedInsideMappingTable && !clickedInsideMappingDialog) {
            if (m_mappingTable && m_mappingTable->selectionModel() && m_mappingTable->selectionModel()->hasSelection()) {
                m_mappingTable->clearSelection();
                    m_mappingTable->setCurrentCell(-1, -1);
                    m_mappingTable->clearFocus();
                    this->setFocus();
                    onMappingTableSelectionChanged();
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
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
            qWarning() << "Application icon not found in resources! Using plain blue icon";
            QPixmap pixmap(32, 32);
            pixmap.fill(QColor(64, 128, 255));
            cachedIcon = QIcon(pixmap);
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
        qDebug() << "Auto-start enabled:" << appPath;
    } else {
        settings.remove("KtoMIDI");
        qDebug() << "Auto-start disabled";
    }
    
    settings.sync();
    
    if (enabled && !settings.contains("KtoMIDI")) {
        qWarning() << "Failed to write auto-start registry entry";
        showMessage("Auto-Start Error", 
                    "Failed to enable auto-start. Please check your Windows registry permissions.",
                    QSystemTrayIcon::Warning);
    }
    
    if (m_autoStartCheck && m_autoStartCheck->isChecked() != enabled) {
        m_autoStartCheck->blockSignals(true);
        m_autoStartCheck->setChecked(enabled);
        m_autoStartCheck->blockSignals(false);
    }
    
    saveSettings();
}

bool MainWindow::isAutoStartEnabled() const
{
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                       QSettings::NativeFormat);
    return settings.contains("KtoMIDI");
}
void MainWindow::updateAutoStartPath()
{
    if (!isAutoStartEnabled()) {
        return;
    }
    
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                       QSettings::NativeFormat);
    QString registryPath = settings.value("KtoMIDI").toString();
    
    QString currentPath = QCoreApplication::applicationFilePath();
    QString expectedPath = QString("\"%1\" --minimized").arg(QDir::toNativeSeparators(currentPath));
    
    if (registryPath != expectedPath) {
        settings.setValue("KtoMIDI", expectedPath);
        settings.sync();
        
        if (settings.value("KtoMIDI").toString() == expectedPath) {
            qDebug() << "Auto-start path updated from:" << registryPath << "to:" << expectedPath;
        } else {
            qWarning() << "Failed to update auto-start path in registry";
        }
    }
}
