#pragma once

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QSplitter>
#include <QPointer>

#include "KeyHook.h"
#include "MidiEngine.h"
#include "KeyMapping.h"
#include "InputMonitor.h"
#include "MappingDialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onKeyPressed(int vkCode, bool isKeyDown, bool isRepeat);
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showMainWindow();
    void quitApplication();
    void onTabChanged(int index);
    
    void refreshMidiPorts();
    void onMidiPortChanged(int index);
    void onMidiPortOpened(const QString &portName);
    void onMidiPortClosed();
    void onMidiError(const QString &error);
    
    void addKeyMapping();
    void removeKeyMapping();
    void editKeyMapping();
    void onMappingTableSelectionChanged();
    void onMidiMessageTriggered(const MidiMessage &message, int vkCode, bool isKeyDown);
    
    void onMappingDialogKeyDetectionRequested();
    
    void loadSettings();
    void saveSettings();
    
    void setAutoStartEnabled(bool enabled);
    bool isAutoStartEnabled() const;
    void updateAutoStartPath();

private:
    void setupUI();
    void setupSystemTray();
    void setupConfigurationTab();
    void setupInputMonitorTab();
    void setupMidiControls();
    void setupSystemControls();
    void setupMappingTable();
    
    void updateMappingTable();
    void updateMidiPortStatus();
    void updateSuppressedKeys();
    
    QString getKeyName(int vkCode) const;
    void showMessage(const QString &title, const QString &message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
    
    QIcon getApplicationIcon() const;
    QIcon getApplicationIcon(const QSize &size) const;

    KeyHook *m_keyHook;
    MidiEngine *m_midiEngine;
    KeyMapping *m_keyMapping;
    InputMonitor *m_inputMonitor;
    
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_showAction;
    QAction *m_quitAction;
    
    QTabWidget *m_tabWidget;
    QWidget *m_configTab;
    QWidget *m_inputMonitorTab;
    
    QGroupBox *m_midiGroup;
    QLabel *m_midiPortLabel;
    QComboBox *m_midiPortCombo;
    QPushButton *m_refreshPortsButton;
    QLabel *m_midiStatusLabel;
    QCheckBox *m_autoConnectCheck;
    
    QGroupBox *m_systemGroup;
    QCheckBox *m_autoStartCheck;
    
    QGroupBox *m_mappingGroup;
    QTableWidget *m_mappingTable;
    QPushButton *m_addMappingButton;
    QPushButton *m_removeMappingButton;
    QPushButton *m_editMappingButton;
    
    int m_currentEditingVkCode;
    bool m_isEditingMapping;
    bool m_waitingForKeyPress;
    MappingDialog *m_currentMappingDialog;
    
    QString m_pendingAutoConnectPort;
    bool m_shouldAutoConnect;
};