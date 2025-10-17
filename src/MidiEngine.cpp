#include "MidiEngine.h"
#include <QDebug>
#include <algorithm>
#include <rtmidi/RtMidi.h>

void MidiMessage::validate() {
    channel = std::clamp(channel, 0, 15);
    note = std::clamp(note, 0, 127);
    velocity = std::clamp(velocity, 0, 127);
    controller = std::clamp(controller, 0, 127);
    value = std::clamp(value, 0, 127);
}

MidiEngine::MidiEngine(QObject *parent)
    : QObject(parent)
    , m_midiOut(nullptr)
    , m_currentPortIndex(-1)
    , m_portOpen(false)
{
    try {
        m_midiOut = std::make_unique<RtMidiOut>();
        refreshPorts();
    } catch (const RtMidiError &error) {
        const QString errorMsg = QString::fromStdString(error.getMessage());
        qCritical() << "RtMidi initialization failed:" << errorMsg;
        emit errorOccurred(QString("Failed to initialize MIDI system: %1").arg(errorMsg));
    }
}

MidiEngine::~MidiEngine()
{
    closePort();
}

QStringList MidiEngine::getAvailablePorts()
{
    refreshPorts();
    return m_availablePorts;
}

void MidiEngine::refreshPorts()
{
    m_availablePorts.clear();
    
    if (!m_midiOut) {
        qWarning() << "MIDI output not initialized";
        return;
    }
    
    try {
        const unsigned int portCount = m_midiOut->getPortCount();
        
        for (unsigned int i = 0; i < portCount; ++i) {
            try {
                const std::string portName = m_midiOut->getPortName(i);
                m_availablePorts.append(QString::fromStdString(portName));
            } catch (const RtMidiError &error) {
                qWarning() << "Error getting port name for index" << i << ":" 
                          << QString::fromStdString(error.getMessage());
            }
        }
    } catch (const RtMidiError &error) {
        const QString errorMsg = QString::fromStdString(error.getMessage());
        qWarning() << "Error refreshing MIDI ports:" << errorMsg;
        emit errorOccurred(QString("Failed to enumerate MIDI ports: %1").arg(errorMsg));
    }
}

bool MidiEngine::openPort(int portIndex)
{
    if (!m_midiOut) {
        const QString errorMsg = "MIDI engine not initialized";
        qCritical() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
    
    closePort();
    
    try {
        refreshPorts();
        
        if (portIndex < 0 || portIndex >= m_availablePorts.size()) {
            const QString errorMsg = QString("Invalid port index: %1 (available: 0-%2)")
                                   .arg(portIndex).arg(m_availablePorts.size() - 1);
            qWarning() << errorMsg;
            emit errorOccurred(errorMsg);
            return false;
        }
        
        m_midiOut->openPort(portIndex);
        m_currentPortIndex = portIndex;
        m_currentPortName = m_availablePorts.at(portIndex);
        m_portOpen = true;
        
        emit portOpened(m_currentPortName);
        return true;
        
    } catch (const RtMidiError &error) {
        const QString errorMsg = QString("Failed to open MIDI port %1: %2")
                              .arg(portIndex)
                              .arg(QString::fromStdString(error.getMessage()));
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
}

bool MidiEngine::openPort(const QString &portName)
{
    refreshPorts();
    const int index = m_availablePorts.indexOf(portName);
    if (index >= 0) {
        return openPort(index);
    } else {
        const QString errorMsg = QString("MIDI port not found: %1").arg(portName);
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return false;
    }
}

void MidiEngine::closePort()
{
    if (m_midiOut && m_portOpen) {
        try {
            m_midiOut->closePort();
        } catch (const RtMidiError &error) {
            qWarning() << "Error closing MIDI port:" << QString::fromStdString(error.getMessage());
        }
    }
    
    m_currentPortIndex = -1;
    m_currentPortName.clear();
    m_portOpen = false;
    emit portClosed();
}

bool MidiEngine::isPortOpen() const
{
    return m_portOpen;
}

QString MidiEngine::getCurrentPortName() const
{
    return m_currentPortName;
}

int MidiEngine::getCurrentPortIndex() const
{
    return m_currentPortIndex;
}

void MidiEngine::sendMidiMessage(const MidiMessage &message)
{
    if (!m_midiOut || !m_portOpen) {
        const QString errorMsg = "Cannot send MIDI: No port open";
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }
    
    try {
        MidiMessage validatedMessage = message;
        validatedMessage.validate();
        
        const std::vector<unsigned char> midiData = createMidiMessage(validatedMessage);
        m_midiOut->sendMessage(&midiData);
        
        emit midiMessageSent(validatedMessage);
        
    } catch (const RtMidiError &error) {
        const QString errorMsg = QString("Failed to send MIDI message: %1")
                              .arg(QString::fromStdString(error.getMessage()));
        qWarning() << errorMsg;
        emit errorOccurred(errorMsg);
    }
}

void MidiEngine::sendNoteOn(int channel, int note, int velocity)
{
    MidiMessage message;
    message.type = MidiMessage::NOTE_ON;
    message.channel = channel;
    message.note = note;
    message.velocity = velocity;
    sendMidiMessage(message);
}

void MidiEngine::sendNoteOff(int channel, int note, int velocity)
{
    MidiMessage message;
    message.type = MidiMessage::NOTE_OFF;
    message.channel = channel;
    message.note = note;
    message.velocity = velocity;
    sendMidiMessage(message);
}

void MidiEngine::sendControlChange(int channel, int controller, int value)
{
    MidiMessage message;
    message.type = MidiMessage::CONTROL_CHANGE;
    message.channel = channel;
    message.controller = controller;
    message.value = value;
    sendMidiMessage(message);
}

std::vector<unsigned char> MidiEngine::createMidiMessage(const MidiMessage &message)
{
    std::vector<unsigned char> midiData;
    
    const int channel = message.channel;
    const int note = message.note;
    const int velocity = message.velocity;
    const int controller = message.controller;
    const int value = message.value;
    
    switch (message.type) {
        case MidiMessage::NOTE_ON:
            midiData.push_back(0x90 | channel);  // Note On + channel
            midiData.push_back(note);
            midiData.push_back(velocity);
            break;
            
        case MidiMessage::NOTE_OFF:
            midiData.push_back(0x80 | channel);  // Note Off + channel
            midiData.push_back(note);
            midiData.push_back(velocity);
            break;
            
        case MidiMessage::CONTROL_CHANGE:
            midiData.push_back(0xB0 | channel);  // Control Change + channel
            midiData.push_back(controller);
            midiData.push_back(value);
            break;
    }
    
    return midiData;
}

QString MidiEngine::midiMessageToString(const MidiMessage &message)
{
    const QString channelStr = QString::number(message.channel + 1);
    
    switch (message.type) {
        case MidiMessage::NOTE_ON:
            return QString("Note On - Ch:%1 Note:%2 Vel:%3")
                   .arg(channelStr)
                   .arg(message.note)
                   .arg(message.velocity);
            
        case MidiMessage::NOTE_OFF:
            return QString("Note Off - Ch:%1 Note:%2 Vel:%3")
                   .arg(channelStr)
                   .arg(message.note)
                   .arg(message.velocity);
            
        case MidiMessage::CONTROL_CHANGE:
            return QString("Control Change - Ch:%1 CC:%2 Val:%3")
                   .arg(channelStr)
                   .arg(message.controller)
                   .arg(message.value);
    }
    
    return "Unknown MIDI Message";
}