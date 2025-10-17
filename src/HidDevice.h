#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QTimer>
#include <windows.h>

extern "C" {
#include <hidsdi.h>
#include <setupapi.h>
}

struct HidDeviceInfo {
    QString name;    QString path;    USHORT vendorId;    USHORT productId;    USHORT usagePage;    USHORT usage;    bool isConnected;    
    HidDeviceInfo() : vendorId(0), productId(0), usagePage(0), usage(0), isConnected(false) {}
};

struct HidInputReport {
    QByteArray data;    int reportId;    quint64 timestamp;};

class HidDevice : public QObject
{
    Q_OBJECT

public:
    explicit HidDevice(QObject *parent = nullptr);
    ~HidDevice();

    static QList<HidDeviceInfo> enumerateDevices();
    
    bool openDevice(const QString &devicePath);
    
    void closeDevice();
    
    bool isOpen() const;
    
    HidDeviceInfo getDeviceInfo() const;
    
    QString getDeviceName() const;
    
    void startMonitoring();
    
    void stopMonitoring();
    
    bool isMonitoring() const;

signals:
    void inputReceived(const HidInputReport &report);
    
    void deviceDisconnected();
    
    void errorOccurred(const QString &error);

private slots:
    void pollDevice();

private:
    void readInputReport();
    
    QString getDeviceString(HANDLE handle, UCHAR stringIndex);
    
    HANDLE m_deviceHandle;    HidDeviceInfo m_deviceInfo;    QTimer *m_pollTimer;    bool m_isMonitoring;    OVERLAPPED m_overlapped;    QByteArray m_inputBuffer;    USHORT m_inputReportLength;};