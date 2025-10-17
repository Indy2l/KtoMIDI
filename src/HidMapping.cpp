#include "HidMapping.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

HidMapping::HidMapping(QObject *parent)
    : QObject(parent)
{
}

HidMapping::~HidMapping()
{
}

void HidMapping::addMapping(const HidMappingEntry &entry)
{
    m_mappings.append(entry);
    emit mappingAdded(entry);
}

void HidMapping::removeMapping(int index)
{
    if (index >= 0 && index < m_mappings.size()) {
        m_mappings.removeAt(index);
        emit mappingRemoved(index);
    }
}

void HidMapping::updateMapping(int index, const HidMappingEntry &entry)
{
    if (index >= 0 && index < m_mappings.size()) {
        m_mappings[index] = entry;
        emit mappingUpdated(entry);
    }
}

HidMappingEntry HidMapping::getMapping(int index) const
{
    if (index >= 0 && index < m_mappings.size()) {
        return m_mappings[index];
    }
    return HidMappingEntry();
}

QList<HidMappingEntry> HidMapping::getAllMappings() const
{
    return m_mappings;
}

void HidMapping::clearAllMappings()
{
    m_mappings.clear();
    m_previousReports.clear();
}

void HidMapping::processHidInput(const QString &devicePath, const QByteArray &data)
{
    QByteArray previousData = m_previousReports.value(devicePath);
    
    for (const HidMappingEntry &mapping : m_mappings) {
        if (!mapping.isEnabled || mapping.devicePath != devicePath) {
            continue;
        }
        
        int currentValue = extractValue(data, mapping.byteIndex, mapping.bitMask);
        int previousValue = extractValue(previousData, mapping.byteIndex, mapping.bitMask);
        
        if (shouldTrigger(mapping, currentValue, previousValue)) {
            emit midiMessageTriggered(mapping.midiMessage);
        }
    }
    
    m_previousReports[devicePath] = data;
}

bool HidMapping::shouldTrigger(const HidMappingEntry &entry, int currentValue, int previousValue) const
{
    switch (entry.triggerType) {
        case HidTriggerType::ValueChange:
            return currentValue != previousValue;
            
        case HidTriggerType::ValueEquals:
            return currentValue == entry.triggerValue && previousValue != entry.triggerValue;
            
        case HidTriggerType::ValueGreater:
            return currentValue > entry.triggerValue && previousValue <= entry.triggerValue;
            
        case HidTriggerType::ValueLess:
            return currentValue < entry.triggerValue && previousValue >= entry.triggerValue;
            
        case HidTriggerType::ButtonPress:
            return previousValue == 0 && currentValue != 0;
            
        case HidTriggerType::ButtonRelease:
            return previousValue != 0 && currentValue == 0;
            
        default:
            return false;
    }
}

int HidMapping::extractValue(const QByteArray &data, int byteIndex, int bitMask) const
{
    if (byteIndex < 0 || byteIndex >= data.size()) {
        return 0;
    }
    
    unsigned char byte = static_cast<unsigned char>(data[byteIndex]);
    return byte & bitMask;
}

bool HidMapping::saveToFile(const QString &filename) const
{
    QJsonArray mappingsArray;
    
    for (const HidMappingEntry &entry : m_mappings) {
        QJsonObject mappingObj;
        mappingObj["devicePath"] = entry.devicePath;
        mappingObj["deviceName"] = entry.deviceName;
        mappingObj["byteIndex"] = entry.byteIndex;
        mappingObj["bitMask"] = entry.bitMask;
        mappingObj["triggerType"] = static_cast<int>(entry.triggerType);
        mappingObj["triggerValue"] = entry.triggerValue;
        mappingObj["isEnabled"] = entry.isEnabled;
        
        QJsonObject midiObj;
        midiObj["type"] = static_cast<int>(entry.midiMessage.type);
        midiObj["channel"] = entry.midiMessage.channel;
        midiObj["note"] = entry.midiMessage.note;
        midiObj["velocity"] = entry.midiMessage.velocity;
        midiObj["controller"] = entry.midiMessage.controller;
        midiObj["value"] = entry.midiMessage.value;
        mappingObj["midiMessage"] = midiObj;
        
        mappingsArray.append(mappingObj);
    }
    
    QJsonObject rootObj;
    rootObj["hidMappings"] = mappingsArray;
    
    QJsonDocument doc(rootObj);
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file for writing:" << filename;
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

bool HidMapping::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open file for reading:" << filename;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qDebug() << "Invalid JSON format in file:" << filename;
        return false;
    }
    
    QJsonObject rootObj = doc.object();
    QJsonArray mappingsArray = rootObj["hidMappings"].toArray();
    
    clearAllMappings();
    
    for (const QJsonValue &value : mappingsArray) {
        QJsonObject mappingObj = value.toObject();
        
        HidMappingEntry entry;
        entry.devicePath = mappingObj["devicePath"].toString();
        entry.deviceName = mappingObj["deviceName"].toString();
        entry.byteIndex = mappingObj["byteIndex"].toInt();
        entry.bitMask = mappingObj["bitMask"].toInt();
        entry.triggerType = static_cast<HidTriggerType>(mappingObj["triggerType"].toInt());
        entry.triggerValue = mappingObj["triggerValue"].toInt();
        entry.isEnabled = mappingObj["isEnabled"].toBool();
        
        QJsonObject midiObj = mappingObj["midiMessage"].toObject();
        entry.midiMessage.type = static_cast<MidiMessage::Type>(midiObj["type"].toInt());
        entry.midiMessage.channel = midiObj["channel"].toInt();
        entry.midiMessage.note = midiObj["note"].toInt();
        entry.midiMessage.velocity = midiObj["velocity"].toInt();
        entry.midiMessage.controller = midiObj["controller"].toInt();
        entry.midiMessage.value = midiObj["value"].toInt();
        
        addMapping(entry);
    }
    
    return true;
}