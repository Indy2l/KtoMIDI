#pragma once

#include <QObject>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include "MidiEngine.h"

struct KeyMappingEntry {
    int vkCode;
    QString keyName;
    bool enableKeyDown;
    bool enableKeyUp;
    bool filterRepeats;
    bool suppressRepeats;
    MidiMessage keyDownMessage;
    MidiMessage keyUpMessage;
    
    KeyMappingEntry() : vkCode(0), enableKeyDown(true), enableKeyUp(false), filterRepeats(true), suppressRepeats(false) {}
};

class KeyMapping : public QObject
{
    Q_OBJECT

public:
    explicit KeyMapping(QObject *parent = nullptr);
    ~KeyMapping();

    void addMapping(const KeyMappingEntry &entry);
    
    void removeMapping(int vkCode);
    
    void updateMapping(const KeyMappingEntry &entry);
    
    void replaceMapping(int oldVkCode, const KeyMappingEntry &newEntry);
    
    KeyMappingEntry getMapping(int vkCode) const;
    
    bool hasMapping(int vkCode) const;
    
    QList<KeyMappingEntry> getAllMappings() const;
    
    void clearAllMappings();

    void processKeyEvent(int vkCode, bool isKeyDown, bool isRepeat);

    QJsonDocument toJson() const;
    
    bool fromJson(const QJsonDocument &doc);
    
    bool saveToFile(const QString &filename) const;
    
    bool loadFromFile(const QString &filename);

signals:
    void mappingAdded(const KeyMappingEntry &entry);
    
    void mappingRemoved(int vkCode);
    
    void mappingUpdated(const KeyMappingEntry &entry);
    
    void midiMessageTriggered(const MidiMessage &message, int vkCode, bool isKeyDown);

private:
    KeyMappingEntry jsonToEntry(const QJsonObject &obj) const;
    
    QJsonObject entryToJson(const KeyMappingEntry &entry) const;
    
    MidiMessage jsonToMidiMessage(const QJsonObject &obj) const;
    
    QJsonObject midiMessageToJson(const MidiMessage &message) const;

    QMap<int, KeyMappingEntry> m_mappings;
};