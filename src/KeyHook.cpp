#include "KeyHook.h"
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>

namespace {
    QMutex s_instanceMutex;
}

KeyHook* KeyHook::s_instance = nullptr;

KeyHook::KeyHook(QObject *parent)
    : QObject(parent)
    , m_keyboardHook(nullptr)
    , m_hookInstalled(false)
{
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance != nullptr) {
        qWarning() << "Multiple KeyHook instances detected - this may cause issues";
    }
    s_instance = this;
}

KeyHook::~KeyHook()
{
    uninstallHook();
    
    QMutexLocker locker(&s_instanceMutex);
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool KeyHook::installHook()
{
    if (m_hookInstalled) {
        return true;
    }

    m_keyboardHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandle(nullptr),
        0
    );

    if (m_keyboardHook != nullptr) {
        m_hookInstalled = true;
        return true;
    } else {
        const DWORD error = GetLastError();
        qCritical() << "Failed to install keyboard hook. Error code:" << error;
        return false;
    }
}

void KeyHook::uninstallHook()
{
    if (m_hookInstalled && m_keyboardHook != nullptr) {
        if (!UnhookWindowsHookEx(m_keyboardHook)) {
            const DWORD error = GetLastError();
            qWarning() << "Failed to uninstall keyboard hook. Error code:" << error;
        }
        
        m_keyboardHook = nullptr;
        m_hookInstalled = false;
    }

    m_pressedKeys.clear();
}

bool KeyHook::isHookInstalled() const
{
    return m_hookInstalled;
}

void KeyHook::setSuppressedRepeatKeys(const QSet<int> &vkCodes)
{
    m_suppressedRepeatKeys = vkCodes;
}

LRESULT CALLBACK KeyHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    QMutexLocker locker(&s_instanceMutex);
    if (nCode >= 0 && s_instance != nullptr) {
        const KBDLLHOOKSTRUCT* pkbhs = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        const int vkCode = static_cast<int>(pkbhs->vkCode);
        const bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        bool isRepeat = false;

        if (isKeyDown) {
            isRepeat = s_instance->updateRepeatState(vkCode, true);
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            s_instance->updateRepeatState(vkCode, false);
        }

        if (s_instance->shouldSuppressKey(vkCode, isRepeat)) {
            s_instance->processKeyEvent(vkCode, isKeyDown, isRepeat);
            return 1;
        }

        s_instance->processKeyEvent(vkCode, isKeyDown, isRepeat);
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool KeyHook::shouldSuppressKey(int vkCode, bool isRepeat) const
{
    return isRepeat && m_suppressedRepeatKeys.contains(vkCode);
}

void KeyHook::processKeyEvent(int vkCode, bool isKeyDown, bool isRepeat)
{
    emit keyPressed(vkCode, isKeyDown, isRepeat);
}

bool KeyHook::updateRepeatState(int vkCode, bool isKeyDown)
{
    if (isKeyDown) {
        if (m_pressedKeys.contains(vkCode)) {
            return true;
        }
        m_pressedKeys.insert(vkCode);
    } else {
        m_pressedKeys.remove(vkCode);
    }

    return false;
}