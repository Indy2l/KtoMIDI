#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QList>
#include "MidiEngine.h"

enum class HidTriggerType {
    ValueChange,    ValueEquals,    ValueGreater,    ValueLess,    ButtonPress,    ButtonRelease};

struct HidMappingEntry {
    QString devicePath;    QString deviceName;    int byteIndex;    int bitMask;    HidTriggerType triggerType;    int triggerValue;    MidiMessage midiMessage;    bool isEnabled;    
    HidMappingEntry() 
        : byteIndex(0), bitMask(0xFF), triggerType(HidTriggerType::ValueChange)
        , triggerValue(0), isEnabled(true) {}
};

class HidMapping : public QObject
{
    Q_OBJECT

public:
    explicit HidMapping(QObject *parent = nullptr);
    ~HidMapping();

    void addMapping(const HidMappingEntry &entry);
    
    void removeMapping(int index);
    
    void updateMapping(int index, const HidMappingEntry &entry);
    
    HidMappingEntry getMapping(int index) const;
    
    QList<HidMappingEntry> getAllMappings() const;
    
    void clearAllMappings();

    void processHidInput(const QString &devicePath, const QByteArray &data);

    bool saveToFile(const QString &filename) const;
    
    bool loadFromFile(const QString &filename);

signals:
    void mappingAdded(const HidMappingEntry &entry);
    
    void mappingRemoved(int index);
    
    void mappingUpdated(const HidMappingEntry &entry);
    
    void midiMessageTriggered(const MidiMessage &message);

private:
    bool shouldTrigger(const HidMappingEntry &entry, int currentValue, int previousValue) const;
    
    int extractValue(const QByteArray &data, int byteIndex, int bitMask) const;
    
    QList<HidMappingEntry> m_mappings;    QHash<QString, QByteArray> m_previousReports;};