#include "KeyMapping.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

KeyMapping::KeyMapping(QObject *parent)
    : QObject(parent)
{
}

KeyMapping::~KeyMapping()
{
}

void KeyMapping::addMapping(const KeyMappingEntry &entry)
{
    m_mappings[entry.vkCode] = entry;
    emit mappingAdded(entry);
}

void KeyMapping::removeMapping(int vkCode)
{
    if (m_mappings.contains(vkCode)) {
        m_mappings.remove(vkCode);
        emit mappingRemoved(vkCode);
    }
}

void KeyMapping::updateMapping(const KeyMappingEntry &entry)
{
    if (m_mappings.contains(entry.vkCode)) {
        m_mappings[entry.vkCode] = entry;
        emit mappingUpdated(entry);
    }
}

void KeyMapping::replaceMapping(int oldVkCode, const KeyMappingEntry &newEntry)
{
    if (m_mappings.contains(oldVkCode)) {
        m_mappings.remove(oldVkCode);
        emit mappingRemoved(oldVkCode);
    }
    
    m_mappings[newEntry.vkCode] = newEntry;
    emit mappingAdded(newEntry);
}

KeyMappingEntry KeyMapping::getMapping(int vkCode) const
{
    return m_mappings.value(vkCode, KeyMappingEntry());
}

bool KeyMapping::hasMapping(int vkCode) const
{
    return m_mappings.contains(vkCode);
}

QList<KeyMappingEntry> KeyMapping::getAllMappings() const
{
    return m_mappings.values();
}

void KeyMapping::clearAllMappings()
{
    const QList<int> vkCodes = m_mappings.keys();
    m_mappings.clear();
    
    for (int vkCode : vkCodes) {
        emit mappingRemoved(vkCode);
    }
}

void KeyMapping::processKeyEvent(int vkCode, bool isKeyDown, bool isRepeat)
{
    if (!hasMapping(vkCode)) {
        return;
    }
    
    KeyMappingEntry entry = getMapping(vkCode);
    
    if (isRepeat && entry.filterRepeats) {
        return;
    }
    
    MidiMessage message;
    bool shouldSend = false;
    
    if (isKeyDown && entry.enableKeyDown) {
        message = entry.keyDownMessage;
        shouldSend = true;
    } else if (!isKeyDown && entry.enableKeyUp) {
        message = entry.keyUpMessage;
        shouldSend = true;
    }
    
    if (shouldSend) {
        emit midiMessageTriggered(message, vkCode, isKeyDown);
    }
}

QJsonDocument KeyMapping::toJson() const
{
    QJsonArray mappingsArray;
    
    for (auto it = m_mappings.constBegin(); it != m_mappings.constEnd(); ++it) {
        mappingsArray.append(entryToJson(it.value()));
    }
    
    QJsonObject rootObject;
    rootObject["version"] = "1.0";
    rootObject["mappings"] = mappingsArray;
    
    return QJsonDocument(rootObject);
}

bool KeyMapping::fromJson(const QJsonDocument &doc)
{
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject rootObject = doc.object();
    
    if (!rootObject.contains("mappings") || !rootObject["mappings"].isArray()) {
        return false;
    }
    
    clearAllMappings();
    
    QJsonArray mappingsArray = rootObject["mappings"].toArray();
    
    for (const QJsonValue &value : mappingsArray) {
        if (value.isObject()) {
            KeyMappingEntry entry = jsonToEntry(value.toObject());
            if (entry.vkCode > 0) {
                addMapping(entry);
            }
        }
    }
    
    return true;
}

bool KeyMapping::saveToFile(const QString &filename) const
{
    QJsonDocument doc = toJson();
    
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    qint64 bytesWritten = file.write(doc.toJson());
    file.close();
    
    return bytesWritten > 0;
}

bool KeyMapping::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    return fromJson(doc);
}

KeyMappingEntry KeyMapping::jsonToEntry(const QJsonObject &obj) const
{
    KeyMappingEntry entry;
    
    entry.vkCode = obj["vkCode"].toInt();
    entry.keyName = obj["keyName"].toString();
    entry.enableKeyDown = obj["enableKeyDown"].toBool(true);
    entry.enableKeyUp = obj["enableKeyUp"].toBool(false);
    entry.filterRepeats = obj["filterRepeats"].toBool(true);
    entry.suppressRepeats = obj["suppressRepeats"].toBool(false);
    
    if (obj.contains("keyDownMessage") && obj["keyDownMessage"].isObject()) {
        entry.keyDownMessage = jsonToMidiMessage(obj["keyDownMessage"].toObject());
    }
    
    if (obj.contains("keyUpMessage") && obj["keyUpMessage"].isObject()) {
        entry.keyUpMessage = jsonToMidiMessage(obj["keyUpMessage"].toObject());
    }
    
    return entry;
}

QJsonObject KeyMapping::entryToJson(const KeyMappingEntry &entry) const
{
    QJsonObject obj;
    
    obj["vkCode"] = entry.vkCode;
    obj["keyName"] = entry.keyName;
    obj["enableKeyDown"] = entry.enableKeyDown;
    obj["enableKeyUp"] = entry.enableKeyUp;
    obj["filterRepeats"] = entry.filterRepeats;
    obj["suppressRepeats"] = entry.suppressRepeats;
    obj["keyDownMessage"] = midiMessageToJson(entry.keyDownMessage);
    obj["keyUpMessage"] = midiMessageToJson(entry.keyUpMessage);
    
    return obj;
}

MidiMessage KeyMapping::jsonToMidiMessage(const QJsonObject &obj) const
{
    MidiMessage message;
    
    message.channel = obj["channel"].toInt(0);
    message.note = obj["note"].toInt(60);
    message.velocity = obj["velocity"].toInt(127);
    message.controller = obj["controller"].toInt(1);
    message.value = obj["value"].toInt(64);
    
    QString typeStr = obj["type"].toString("NOTE_ON");
    if (typeStr == "NOTE_OFF") {
        message.type = MidiMessage::NOTE_OFF;
    } else if (typeStr == "CONTROL_CHANGE") {
        message.type = MidiMessage::CONTROL_CHANGE;
    } else {
        message.type = MidiMessage::NOTE_ON;
    }
    
    return message;
}

QJsonObject KeyMapping::midiMessageToJson(const MidiMessage &message) const
{
    QJsonObject obj;
    
    obj["channel"] = message.channel;
    obj["note"] = message.note;
    obj["velocity"] = message.velocity;
    obj["controller"] = message.controller;
    obj["value"] = message.value;
    
    QString typeStr;
    switch (message.type) {
        case MidiMessage::NOTE_ON:
            typeStr = "NOTE_ON";
            break;
        case MidiMessage::NOTE_OFF:
            typeStr = "NOTE_OFF";
            break;
        case MidiMessage::CONTROL_CHANGE:
            typeStr = "CONTROL_CHANGE";
            break;
    }
    obj["type"] = typeStr;
    
    return obj;
}