#pragma once

#include <QObject>
#include <QSet>
#include <QMutex>
#include <windows.h>

class KeyHook : public QObject
{
    Q_OBJECT

public:
    explicit KeyHook(QObject *parent = nullptr);
    ~KeyHook();

    bool installHook();
    
    void uninstallHook();
    
    bool isHookInstalled() const;
    
    void setSuppressedRepeatKeys(const QSet<int> &vkCodes);

signals:
    void keyPressed(int vkCode, bool isKeyDown, bool isRepeat);

private:
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
    
    static KeyHook* s_instance;
    
    void processKeyEvent(int vkCode, bool isKeyDown, bool isRepeat);

    Q_INVOKABLE void emitKeyPressed(int vkCode, bool isKeyDown, bool isRepeat);
    bool handleHookEvent(WPARAM wParam, const KBDLLHOOKSTRUCT *pkbhs);
    bool updateRepeatState(int vkCode, bool isKeyDown);
    bool shouldSuppressKey(int vkCode, bool isRepeat) const;

    HHOOK m_keyboardHook;
    bool m_hookInstalled;
    QSet<int> m_suppressedRepeatKeys;
    QSet<int> m_pressedKeys;
    mutable QMutex m_stateMutex;
};