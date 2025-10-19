#include "MappingDialog.h"
#include "KeyUtils.h"
#include <QIntValidator>
#include <windows.h>

namespace {
    constexpr int DEFAULT_CHANNEL = 0;
    constexpr int DEFAULT_NOTE = 60;
    constexpr int DEFAULT_VELOCITY = 100;
    constexpr int DEFAULT_CONTROLLER = 1;
    constexpr int DEFAULT_VALUE = 127;
    
    constexpr int DIALOG_MIN_WIDTH = 500;
    constexpr int DIALOG_MIN_HEIGHT = 650;
    constexpr int DIALOG_DEFAULT_WIDTH = 520;
    constexpr int DIALOG_DEFAULT_HEIGHT = 680;
}

MappingDialog::MappingDialog(QWidget *parent)
    : QDialog(parent)
    , m_isListening(false)
    , m_isEditing(false)
{
    setWindowTitle("Add Key Mapping");
    setupUI();
    
    m_enableKeyDownCheck->setChecked(true);
    m_enableKeyUpCheck->setChecked(false);
    updateControlVisibility();
}

MappingDialog::MappingDialog(const KeyMappingEntry &entry, QWidget *parent)
    : QDialog(parent)
    , m_isListening(false)
    , m_isEditing(true)
{
    setWindowTitle("Edit Key Mapping");
    setupUI();
    setMappingEntry(entry);
}

MappingDialog::~MappingDialog()
{
}

void MappingDialog::setupUI()
{
    setModal(true);
    setMinimumSize(DIALOG_MIN_WIDTH, DIALOG_MIN_HEIGHT);
    resize(DIALOG_DEFAULT_WIDTH, DIALOG_DEFAULT_HEIGHT);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    setupKeyDetectionGroup();
    setupKeyDownGroup();
    setupKeyUpGroup();
    
    mainLayout->addWidget(m_keyDetectionGroup);
    
    QHBoxLayout *enableLayout = new QHBoxLayout();
    enableLayout->setSpacing(20);
    
    m_enableKeyDownCheck = new QCheckBox("Enable Key Press MIDI");
    m_enableKeyDownCheck->setToolTip("Send MIDI message when key is pressed down");
    connect(m_enableKeyDownCheck, &QCheckBox::toggled, this, &MappingDialog::onEnableKeyDownToggled);
    enableLayout->addWidget(m_enableKeyDownCheck);
    
    m_enableKeyUpCheck = new QCheckBox("Enable Key Release MIDI");
    m_enableKeyUpCheck->setToolTip("Send MIDI message when key is released");
    connect(m_enableKeyUpCheck, &QCheckBox::toggled, this, &MappingDialog::onEnableKeyUpToggled);
    enableLayout->addWidget(m_enableKeyUpCheck);
    
    enableLayout->addStretch();
    mainLayout->addLayout(enableLayout);
    
    m_filterRepeatsCheck = new QCheckBox("Filter Repeated Keys (App-Level)");
    m_filterRepeatsCheck->setChecked(true);
    m_filterRepeatsCheck->setToolTip("Prevent MIDI messages from being sent when this key is held down and auto-repeating");
    mainLayout->addWidget(m_filterRepeatsCheck);
    
    m_suppressRepeatsCheck = new QCheckBox("Block Repeated Keys (System-Wide)");
    m_suppressRepeatsCheck->setChecked(false);
    m_suppressRepeatsCheck->setToolTip("Completely block this key from auto-repeating anywhere in the system when held down");
    mainLayout->addWidget(m_suppressRepeatsCheck);
    
    mainLayout->addWidget(m_keyDownGroup);
    mainLayout->addWidget(m_keyUpGroup);
    
    mainLayout->addStretch();
    
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
    
    setLayout(mainLayout);
}

void MappingDialog::setupKeyDetectionGroup()
{
    m_keyDetectionGroup = new QGroupBox("Key Detection", this);
    m_keyDetectionGroup->setToolTip("Configure which keyboard key to map");
    
    QGridLayout *layout = new QGridLayout(m_keyDetectionGroup);
    layout->setSpacing(8);
    layout->setContentsMargins(10, 15, 10, 10);
    
    layout->addWidget(new QLabel("Virtual Key Code:"), 0, 0);
    m_vkCodeEdit = new QLineEdit();
    m_vkCodeEdit->setValidator(new QIntValidator(1, 255, this));
    m_vkCodeEdit->setPlaceholderText("Enter VK code (1-255)");
    m_vkCodeEdit->setToolTip("Windows virtual key code (VK_* constants)");
    connect(m_vkCodeEdit, &QLineEdit::textChanged, this, &MappingDialog::onVkCodeChanged);
    layout->addWidget(m_vkCodeEdit, 0, 1);
    
    m_listenButton = new QPushButton("Listen for Key");
    m_listenButton->setToolTip("Click and then press a key to detect its VK code");
    connect(m_listenButton, &QPushButton::clicked, this, &MappingDialog::onListenButtonClicked);
    layout->addWidget(m_listenButton, 0, 2);
    
    layout->addWidget(new QLabel("Key Name:"), 1, 0);
    m_keyNameEdit = new QLineEdit();
    m_keyNameEdit->setReadOnly(true);
    m_keyNameEdit->setToolTip("Human-readable name for the selected key");
    m_keyNameEdit->setPlaceholderText("Key name will appear here");
    layout->addWidget(m_keyNameEdit, 1, 1, 1, 2);
}

void MappingDialog::setupKeyDownGroup()
{
    m_keyDownGroup = new QGroupBox("KeyDown MIDI Message");
    QGridLayout *layout = new QGridLayout(m_keyDownGroup);
    
    layout->addWidget(new QLabel("Type:"), 0, 0);
    m_keyDownTypeCombo = new QComboBox();
    m_keyDownTypeCombo->addItems({"Note On", "Note Off", "Control Change"});
    connect(m_keyDownTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MappingDialog::updateKeyDownControlVisibility);
    layout->addWidget(m_keyDownTypeCombo, 0, 1);
    
    layout->addWidget(new QLabel("Channel:"), 0, 2);
    m_keyDownChannelSpin = new QSpinBox();
    m_keyDownChannelSpin->setRange(1, 16);
    m_keyDownChannelSpin->setValue(1);
    layout->addWidget(m_keyDownChannelSpin, 0, 3);
    
    m_keyDownNoteLabel = new QLabel("Note:");
    layout->addWidget(m_keyDownNoteLabel, 1, 0);
    m_keyDownNoteSpin = new QSpinBox();
    m_keyDownNoteSpin->setRange(0, 127);
    m_keyDownNoteSpin->setValue(60);
    layout->addWidget(m_keyDownNoteSpin, 1, 1);
    
    m_keyDownVelocityLabel = new QLabel("Velocity:");
    layout->addWidget(m_keyDownVelocityLabel, 1, 2);
    m_keyDownVelocitySpin = new QSpinBox();
    m_keyDownVelocitySpin->setRange(0, 127);
    m_keyDownVelocitySpin->setValue(127);
    layout->addWidget(m_keyDownVelocitySpin, 1, 3);
    
    m_keyDownControllerLabel = new QLabel("Controller:");
    layout->addWidget(m_keyDownControllerLabel, 2, 0);
    m_keyDownControllerSpin = new QSpinBox();
    m_keyDownControllerSpin->setRange(0, 127);
    m_keyDownControllerSpin->setValue(1);
    layout->addWidget(m_keyDownControllerSpin, 2, 1);
    
    m_keyDownValueLabel = new QLabel("Value:");
    layout->addWidget(m_keyDownValueLabel, 2, 2);
    m_keyDownValueSpin = new QSpinBox();
    m_keyDownValueSpin->setRange(0, 127);
    m_keyDownValueSpin->setValue(127);
    layout->addWidget(m_keyDownValueSpin, 2, 3);
}

void MappingDialog::setupKeyUpGroup()
{
    m_keyUpGroup = new QGroupBox("KeyUp MIDI Message");
    QGridLayout *layout = new QGridLayout(m_keyUpGroup);
    
    layout->addWidget(new QLabel("Type:"), 0, 0);
    m_keyUpTypeCombo = new QComboBox();
    m_keyUpTypeCombo->addItems({"Note On", "Note Off", "Control Change"});
    m_keyUpTypeCombo->setCurrentIndex(1);
    connect(m_keyUpTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MappingDialog::updateKeyUpControlVisibility);
    layout->addWidget(m_keyUpTypeCombo, 0, 1);
    
    layout->addWidget(new QLabel("Channel:"), 0, 2);
    m_keyUpChannelSpin = new QSpinBox();
    m_keyUpChannelSpin->setRange(1, 16);
    m_keyUpChannelSpin->setValue(1);
    layout->addWidget(m_keyUpChannelSpin, 0, 3);
    
    m_keyUpNoteLabel = new QLabel("Note:");
    layout->addWidget(m_keyUpNoteLabel, 1, 0);
    m_keyUpNoteSpin = new QSpinBox();
    m_keyUpNoteSpin->setRange(0, 127);
    m_keyUpNoteSpin->setValue(60);
    layout->addWidget(m_keyUpNoteSpin, 1, 1);
    
    m_keyUpVelocityLabel = new QLabel("Velocity:");
    layout->addWidget(m_keyUpVelocityLabel, 1, 2);
    m_keyUpVelocitySpin = new QSpinBox();
    m_keyUpVelocitySpin->setRange(0, 127);
    m_keyUpVelocitySpin->setValue(64);
    layout->addWidget(m_keyUpVelocitySpin, 1, 3);
    
    m_keyUpControllerLabel = new QLabel("Controller:");
    layout->addWidget(m_keyUpControllerLabel, 2, 0);
    m_keyUpControllerSpin = new QSpinBox();
    m_keyUpControllerSpin->setRange(0, 127);
    m_keyUpControllerSpin->setValue(1);
    layout->addWidget(m_keyUpControllerSpin, 2, 1);
    
    m_keyUpValueLabel = new QLabel("Value:");
    layout->addWidget(m_keyUpValueLabel, 2, 2);
    m_keyUpValueSpin = new QSpinBox();
    m_keyUpValueSpin->setRange(0, 127);
    m_keyUpValueSpin->setValue(0);
    layout->addWidget(m_keyUpValueSpin, 2, 3);
}

void MappingDialog::onListenButtonClicked()
{
    if (m_isListening) {
        m_isListening = false;
        m_listenButton->setText("Listen for Key");
        m_listenButton->setStyleSheet("");
    } else {
        m_isListening = true;
        m_listenButton->setText("Stop Listening");
        
        QPalette palette = m_listenButton->palette();
        QColor highlightColor = palette.color(QPalette::Highlight);
        QColor highlightTextColor = palette.color(QPalette::HighlightedText);
        
        m_listenButton->setStyleSheet(
            QString("background-color: %1; color: %2;")
            .arg(highlightColor.name())
            .arg(highlightTextColor.name())
        );
        
        emit keyDetectionRequested();
    }
}

void MappingDialog::onVkCodeChanged()
{
    updateKeyName();
}

void MappingDialog::onEnableKeyDownToggled(bool enabled)
{
    m_keyDownGroup->setEnabled(enabled);
}

void MappingDialog::onEnableKeyUpToggled(bool enabled)
{
    m_keyUpGroup->setEnabled(enabled);
}

void MappingDialog::setDetectedVkCode(int vkCode)
{
    if (m_isListening) {
        m_vkCodeEdit->setText(QString::number(vkCode));
        m_isListening = false;
        m_listenButton->setText("Listen for Key");
        m_listenButton->setStyleSheet("");
        updateKeyName();
    }
}

void MappingDialog::updateKeyName()
{
    bool ok;
    int vkCode = m_vkCodeEdit->text().toInt(&ok);
    
    if (ok && vkCode >= 1 && vkCode <= 255) {
        QString keyName = getKeyName(vkCode);
        m_keyNameEdit->setText(keyName);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_keyNameEdit->setText("");
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

QString MappingDialog::getKeyName(int vkCode) const
{
    return KeyUtils::getKeyName(vkCode);
}

void MappingDialog::updateControlVisibility()
{
    m_keyDownGroup->setEnabled(m_enableKeyDownCheck->isChecked());
    m_keyUpGroup->setEnabled(m_enableKeyUpCheck->isChecked());
    
    updateKeyDownControlVisibility();
    updateKeyUpControlVisibility();
}

KeyMappingEntry MappingDialog::getMappingEntry() const
{
    KeyMappingEntry entry;
    
    bool ok;
    int vkCode = m_vkCodeEdit->text().toInt(&ok);
    if (!ok || vkCode < 1 || vkCode > 255) {
        return entry;
    }
    
    entry.vkCode = vkCode;
    entry.keyName = m_keyNameEdit->text();
    entry.enableKeyDown = m_enableKeyDownCheck->isChecked();
    entry.enableKeyUp = m_enableKeyUpCheck->isChecked();
    entry.filterRepeats = m_filterRepeatsCheck->isChecked();
    entry.suppressRepeats = m_suppressRepeatsCheck->isChecked();
    
    entry.keyDownMessage.type = static_cast<MidiMessage::Type>(m_keyDownTypeCombo->currentIndex());
    entry.keyDownMessage.channel = m_keyDownChannelSpin->value() - 1;
    entry.keyDownMessage.note = m_keyDownNoteSpin->value();
    entry.keyDownMessage.velocity = m_keyDownVelocitySpin->value();
    entry.keyDownMessage.controller = m_keyDownControllerSpin->value();
    entry.keyDownMessage.value = m_keyDownValueSpin->value();
    
    entry.keyUpMessage.type = static_cast<MidiMessage::Type>(m_keyUpTypeCombo->currentIndex());
    entry.keyUpMessage.channel = m_keyUpChannelSpin->value() - 1;
    entry.keyUpMessage.note = m_keyUpNoteSpin->value();
    entry.keyUpMessage.velocity = m_keyUpVelocitySpin->value();
    entry.keyUpMessage.controller = m_keyUpControllerSpin->value();
    entry.keyUpMessage.value = m_keyUpValueSpin->value();
    
    return entry;
}

void MappingDialog::setMappingEntry(const KeyMappingEntry &entry)
{
    m_vkCodeEdit->blockSignals(true);
    m_enableKeyDownCheck->blockSignals(true);
    m_enableKeyUpCheck->blockSignals(true);
    
    m_vkCodeEdit->setText(QString::number(entry.vkCode));
    m_keyNameEdit->setText(entry.keyName);
    m_enableKeyDownCheck->setChecked(entry.enableKeyDown);
    m_enableKeyUpCheck->setChecked(entry.enableKeyUp);
    m_filterRepeatsCheck->setChecked(entry.filterRepeats);
    m_suppressRepeatsCheck->setChecked(entry.suppressRepeats);
    
    m_keyDownTypeCombo->setCurrentIndex(static_cast<int>(entry.keyDownMessage.type));
    m_keyDownChannelSpin->setValue(entry.keyDownMessage.channel + 1);
    m_keyDownNoteSpin->setValue(entry.keyDownMessage.note);
    m_keyDownVelocitySpin->setValue(entry.keyDownMessage.velocity);
    m_keyDownControllerSpin->setValue(entry.keyDownMessage.controller);
    m_keyDownValueSpin->setValue(entry.keyDownMessage.value);
    
    m_keyUpTypeCombo->setCurrentIndex(static_cast<int>(entry.keyUpMessage.type));
    m_keyUpChannelSpin->setValue(entry.keyUpMessage.channel + 1);
    m_keyUpNoteSpin->setValue(entry.keyUpMessage.note);
    m_keyUpVelocitySpin->setValue(entry.keyUpMessage.velocity);
    m_keyUpControllerSpin->setValue(entry.keyUpMessage.controller);
    m_keyUpValueSpin->setValue(entry.keyUpMessage.value);
    
    m_vkCodeEdit->blockSignals(false);
    m_enableKeyDownCheck->blockSignals(false);
    m_enableKeyUpCheck->blockSignals(false);
    
    updateControlVisibility();
    updateKeyName();
}

void MappingDialog::updateKeyDownControlVisibility()
{
    bool isNoteMessage = (m_keyDownTypeCombo->currentIndex() == 0 || m_keyDownTypeCombo->currentIndex() == 1);
    bool isControlChange = (m_keyDownTypeCombo->currentIndex() == 2);
    
    m_keyDownNoteLabel->setVisible(isNoteMessage);
    m_keyDownNoteSpin->setVisible(isNoteMessage);
    m_keyDownVelocityLabel->setVisible(isNoteMessage);
    m_keyDownVelocitySpin->setVisible(isNoteMessage);
    
    m_keyDownControllerLabel->setVisible(isControlChange);
    m_keyDownControllerSpin->setVisible(isControlChange);
    m_keyDownValueLabel->setVisible(isControlChange);
    m_keyDownValueSpin->setVisible(isControlChange);
}

void MappingDialog::updateKeyUpControlVisibility()
{
    bool isNoteMessage = (m_keyUpTypeCombo->currentIndex() == 0 || m_keyUpTypeCombo->currentIndex() == 1);
    bool isControlChange = (m_keyUpTypeCombo->currentIndex() == 2);
    
    m_keyUpNoteLabel->setVisible(isNoteMessage);
    m_keyUpNoteSpin->setVisible(isNoteMessage);
    m_keyUpVelocityLabel->setVisible(isNoteMessage);
    m_keyUpVelocitySpin->setVisible(isNoteMessage);
    
    m_keyUpControllerLabel->setVisible(isControlChange);
    m_keyUpControllerSpin->setVisible(isControlChange);
    m_keyUpValueLabel->setVisible(isControlChange);
    m_keyUpValueSpin->setVisible(isControlChange);
}