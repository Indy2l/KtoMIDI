#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QString>

class InputMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit InputMonitor(QWidget *parent = nullptr);
    ~InputMonitor();

    void logKeyEvent(int vkCode, bool isKeyDown, bool isRepeat = false);
    
    void setIgnoreRepeats(bool ignore);
    
    bool shouldLogEvents() const;

public slots:
    void clearConsole();
    
    void setLoggingEnabled(bool enabled);

private slots:
    void onIgnoreRepeatsToggled(bool checked);

private:
    void setupUI();
    
    QString formatKeyEvent(int vkCode, bool isKeyDown, bool isRepeat);
    
    QString getKeyName(int vkCode) const;

    QVBoxLayout *m_layout;
    QTextEdit *m_console;
    QPushButton *m_clearButton;
    QCheckBox *m_ignoreRepeatsCheckBox;
    QLabel *m_statusLabel;
    
    bool m_ignoreRepeats;
    bool m_loggingEnabled;
    int m_eventCount;
};