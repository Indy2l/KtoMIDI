#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QDialogButtonBox>
#include "KeyMapping.h"

class MappingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MappingDialog(QWidget *parent = nullptr);
    
    explicit MappingDialog(const KeyMappingEntry &entry, QWidget *parent = nullptr);
    
    ~MappingDialog();

    KeyMappingEntry getMappingEntry() const;
    
    void setMappingEntry(const KeyMappingEntry &entry);

signals:
    void keyDetectionRequested();

private slots:
    void onListenButtonClicked();
    
    void onVkCodeChanged();
    
    void onEnableKeyDownToggled(bool enabled);
    
    void onEnableKeyUpToggled(bool enabled);

public slots:
    void setDetectedVkCode(int vkCode);

private:
    void setupUI();
    
    void setupKeyDetectionGroup();
    
    void setupKeyDownGroup();
    
    void setupKeyUpGroup();
    
    void updateKeyName();
    
    QString getKeyName(int vkCode) const;
    
    void updateControlVisibility();
    
    void updateKeyDownControlVisibility();
    
    void updateKeyUpControlVisibility();

    QGroupBox *m_keyDetectionGroup;    QLineEdit *m_vkCodeEdit;    QPushButton *m_listenButton;    QLineEdit *m_keyNameEdit;
    QCheckBox *m_enableKeyDownCheck;    QCheckBox *m_enableKeyUpCheck;    QCheckBox *m_filterRepeatsCheck;    QCheckBox *m_suppressRepeatsCheck;
    QGroupBox *m_keyDownGroup;    QComboBox *m_keyDownTypeCombo;    QSpinBox *m_keyDownChannelSpin;    QLabel *m_keyDownNoteLabel;    QSpinBox *m_keyDownNoteSpin;    QLabel *m_keyDownVelocityLabel;    QSpinBox *m_keyDownVelocitySpin;    QLabel *m_keyDownControllerLabel;    QSpinBox *m_keyDownControllerSpin;    QLabel *m_keyDownValueLabel;    QSpinBox *m_keyDownValueSpin;
    QGroupBox *m_keyUpGroup;    QComboBox *m_keyUpTypeCombo;    QSpinBox *m_keyUpChannelSpin;    QLabel *m_keyUpNoteLabel;    QSpinBox *m_keyUpNoteSpin;    QLabel *m_keyUpVelocityLabel;    QSpinBox *m_keyUpVelocitySpin;    QLabel *m_keyUpControllerLabel;    QSpinBox *m_keyUpControllerSpin;    QLabel *m_keyUpValueLabel;    QSpinBox *m_keyUpValueSpin;
    QDialogButtonBox *m_buttonBox;
    bool m_isListening;    bool m_isEditing;};