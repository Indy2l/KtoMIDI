#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>

class RtMidiOut;

struct MidiMessage {
    int channel;
    int note;
    int velocity;
    int controller;
    int value;
    
    enum Type { 
        NOTE_ON,
        NOTE_OFF,
        CONTROL_CHANGE
    } type;
    
    MidiMessage() : channel(0), note(60), velocity(127), controller(1), value(64), type(NOTE_ON) {}
    
    void validate();
};

class MidiEngine : public QObject
{
    Q_OBJECT

public:
    explicit MidiEngine(QObject *parent = nullptr);
    ~MidiEngine();

    QStringList getAvailablePorts();
    bool openPort(int portIndex);
    bool openPort(const QString &portName);
    void closePort();
    bool isPortOpen() const;
    QString getCurrentPortName() const;
    int getCurrentPortIndex() const;

    void sendMidiMessage(const MidiMessage &message);
    void sendNoteOn(int channel, int note, int velocity);
    void sendNoteOff(int channel, int note, int velocity);
    void sendControlChange(int channel, int controller, int value);

    static QString midiMessageToString(const MidiMessage &message);

signals:
    void portOpened(const QString &portName);
    void portClosed();
    void midiMessageSent(const MidiMessage &message);
    void errorOccurred(const QString &error);

private:
    void refreshPorts();
    std::vector<unsigned char> createMidiMessage(const MidiMessage &message);

    std::unique_ptr<RtMidiOut> m_midiOut;
    QStringList m_availablePorts;
    int m_currentPortIndex;
    QString m_currentPortName;
    bool m_portOpen;
};