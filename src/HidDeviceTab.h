#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QTimer>
#include "HidDevice.h"
#include "HidMapping.h"
#include "MidiEngine.h"

class HidDeviceTab : public QWidget
{
    Q_OBJECT

public:
    explicit HidDeviceTab(MidiEngine *midiEngine, QWidget *parent = nullptr);
    ~HidDeviceTab();

    void saveSettings();
    void loadSettings();

signals:
    void statusMessage(const QString &message);

private slots:
    void onRefreshDevices();
    void onDeviceSelectionChanged(int index);
    void onConnectDevice();
    void onDisconnectDevice();
    void onStartMonitoring();
    void onStopMonitoring();
    void onAddMapping();
    void onEditMapping();
    void onRemoveMapping();
    void onMappingSelectionChanged();
    void onHidInputReceived(const HidInputReport &report);
    void onDeviceDisconnected();
    void onDeviceError(const QString &error);

private:
    void setupUI();
    void setupDeviceControls();
    void setupMappingControls();
    void setupInputMonitor();
    void updateDeviceList();
    void updateConnectionStatus();
    void updateMappingTable();
    void updateInputDisplay(const QByteArray &inputData);
    QString formatByteArray(const QByteArray &byteData) const;
    
    MidiEngine *m_midiEngine;
    HidDevice *m_hidDevice;
    HidMapping *m_hidMapping;
    
    QGroupBox *m_deviceGroup;
    QComboBox *m_deviceCombo;
    QPushButton *m_refreshButton;
    QPushButton *m_connectButton;
    QPushButton *m_disconnectButton;
    QLabel *m_connectionStatus;
    
    QGroupBox *m_monitorGroup;
    QPushButton *m_startMonitorButton;
    QPushButton *m_stopMonitorButton;
    QTextEdit *m_inputDisplay;
    QPushButton *m_clearDisplayButton;
    
    QGroupBox *m_mappingGroup;
    QTableWidget *m_mappingTable;
    QPushButton *m_addMappingButton;
    QPushButton *m_editMappingButton;
    QPushButton *m_removeMappingButton;
    
    QList<HidDeviceInfo> m_availableDevices;
};