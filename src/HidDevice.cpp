#include "HidDevice.h"
#include <QDebug>
#include <QDateTime>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace {
    constexpr int HID_POLL_INTERVAL_MS = 10;  // Poll HID devices every 10ms (100Hz)
}

HidDevice::HidDevice(QObject *parent)
    : QObject(parent)
    , m_deviceHandle(INVALID_HANDLE_VALUE)
    , m_pollTimer(new QTimer(this))
    , m_isMonitoring(false)
    , m_inputReportLength(0)
{
    connect(m_pollTimer, &QTimer::timeout, this, &HidDevice::pollDevice);
    m_pollTimer->setInterval(HID_POLL_INTERVAL_MS);
    
    ZeroMemory(&m_overlapped, sizeof(m_overlapped));
}

HidDevice::~HidDevice()
{
    closeDevice();
}

QList<HidDeviceInfo> HidDevice::enumerateDevices()
{
    QList<HidDeviceInfo> devices;
    
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, 
                                                  DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        qDebug() << "Failed to get HID device info set";
        return devices;
    }
    
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    
    for (DWORD deviceIndex = 0; ; deviceIndex++) {
        if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &hidGuid, 
                                        deviceIndex, &deviceInterfaceData)) {
            break; // No more devices
        }
        
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData, 
                                       NULL, 0, &requiredSize, NULL);
        
        if (requiredSize == 0) continue;
        
        PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = 
            (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
        deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        
        if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &deviceInterfaceData,
                                           deviceInterfaceDetailData, requiredSize,
                                           NULL, NULL)) {
            
            QString devicePath = QString::fromWCharArray(deviceInterfaceDetailData->DevicePath);
            
            HANDLE handle = CreateFile(deviceInterfaceDetailData->DevicePath,
                                     0, // No access needed for info
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL, OPEN_EXISTING, 0, NULL);
            
            if (handle != INVALID_HANDLE_VALUE) {
                HidDeviceInfo info;
                info.path = devicePath;
                info.isConnected = true;
                
                HIDD_ATTRIBUTES attributes;
                attributes.Size = sizeof(HIDD_ATTRIBUTES);
                if (HidD_GetAttributes(handle, &attributes)) {
                    info.vendorId = attributes.VendorID;
                    info.productId = attributes.ProductID;
                }
                
                PHIDP_PREPARSED_DATA preparsedData;
                if (HidD_GetPreparsedData(handle, &preparsedData)) {
                    HIDP_CAPS caps;
                    if (HidP_GetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS) {
                        info.usagePage = caps.UsagePage;
                        info.usage = caps.Usage;
                    }
                    HidD_FreePreparsedData(preparsedData);
                }
                
                wchar_t productString[256];
                if (HidD_GetProductString(handle, productString, sizeof(productString))) {
                    info.name = QString::fromWCharArray(productString);
                } else {
                    info.name = QString("HID Device (VID:%1 PID:%2)")
                                .arg(info.vendorId, 4, 16, QChar('0'))
                                .arg(info.productId, 4, 16, QChar('0'));
                }
                
                CloseHandle(handle);
                devices.append(info);
            }
        }
        
        free(deviceInterfaceDetailData);
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    
    qDebug() << "Found" << devices.size() << "HID devices";
    return devices;
}

bool HidDevice::openDevice(const QString &devicePath)
{
    closeDevice();
    
    m_deviceHandle = CreateFile(devicePath.toStdWString().c_str(),
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL, OPEN_EXISTING, 
                               FILE_FLAG_OVERLAPPED, NULL);
    
    if (m_deviceHandle == INVALID_HANDLE_VALUE) {
        emit errorOccurred(QString("Failed to open device: %1").arg(devicePath));
        return false;
    }
    
    m_deviceInfo.path = devicePath;
    m_deviceInfo.isConnected = true;
    
    HIDD_ATTRIBUTES attributes;
    attributes.Size = sizeof(HIDD_ATTRIBUTES);
    if (HidD_GetAttributes(m_deviceHandle, &attributes)) {
        m_deviceInfo.vendorId = attributes.VendorID;
        m_deviceInfo.productId = attributes.ProductID;
    }
    
    PHIDP_PREPARSED_DATA preparsedData;
    if (HidD_GetPreparsedData(m_deviceHandle, &preparsedData)) {
        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsedData, &caps) == HIDP_STATUS_SUCCESS) {
            m_deviceInfo.usagePage = caps.UsagePage;
            m_deviceInfo.usage = caps.Usage;
            m_inputReportLength = caps.InputReportByteLength;
        }
        HidD_FreePreparsedData(preparsedData);
    }
    
    wchar_t productString[256];
    if (HidD_GetProductString(m_deviceHandle, productString, sizeof(productString))) {
        m_deviceInfo.name = QString::fromWCharArray(productString);
    } else {
        m_deviceInfo.name = QString("HID Device (VID:%1 PID:%2)")
                            .arg(m_deviceInfo.vendorId, 4, 16, QChar('0'))
                            .arg(m_deviceInfo.productId, 4, 16, QChar('0'));
    }
    
    m_inputBuffer.resize(m_inputReportLength);
    
    return true;
}

void HidDevice::closeDevice()
{
    stopMonitoring();
    
    if (m_deviceHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_deviceHandle);
        m_deviceHandle = INVALID_HANDLE_VALUE;
    }
    
    m_deviceInfo = HidDeviceInfo();
    m_inputReportLength = 0;
}

bool HidDevice::isOpen() const
{
    return m_deviceHandle != INVALID_HANDLE_VALUE;
}

HidDeviceInfo HidDevice::getDeviceInfo() const
{
    return m_deviceInfo;
}

QString HidDevice::getDeviceName() const
{
    return m_deviceInfo.name;
}

void HidDevice::startMonitoring()
{
    if (!isOpen() || m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = true;
    m_pollTimer->start();
}

void HidDevice::stopMonitoring()
{
    if (!m_isMonitoring) {
        return;
    }
    
    m_isMonitoring = false;
    m_pollTimer->stop();
}

bool HidDevice::isMonitoring() const
{
    return m_isMonitoring;
}

void HidDevice::pollDevice()
{
    if (!isOpen()) {
        return;
    }
    
    readInputReport();
}

void HidDevice::readInputReport()
{
    DWORD bytesRead = 0;
    BOOL result = ReadFile(m_deviceHandle, m_inputBuffer.data(), 
                          m_inputReportLength, &bytesRead, &m_overlapped);
    
    if (result || GetLastError() == ERROR_IO_PENDING) {
        if (GetOverlappedResult(m_deviceHandle, &m_overlapped, &bytesRead, FALSE)) {
            if (bytesRead > 0) {
                HidInputReport report;
                report.data = QByteArray(m_inputBuffer.data(), bytesRead);
                report.reportId = bytesRead > 0 ? static_cast<unsigned char>(m_inputBuffer[0]) : 0;
                report.timestamp = QDateTime::currentMSecsSinceEpoch();
                
                emit inputReceived(report);
            }
        }
    } else {
        DWORD error = GetLastError();
        if (error == ERROR_DEVICE_NOT_CONNECTED || error == ERROR_FILE_NOT_FOUND) {
            emit deviceDisconnected();
            stopMonitoring();
        } else {
            emit errorOccurred(QString("Read error: %1").arg(error));
        }
    }
}