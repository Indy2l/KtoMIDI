#pragma once

#include <QString>
#include <windows.h>

namespace KeyUtils {

inline QString getKeyName(int vkCode)
{
    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    
    if (vkCode == VK_LEFT || vkCode == VK_UP || vkCode == VK_RIGHT || vkCode == VK_DOWN ||
        vkCode == VK_PRIOR || vkCode == VK_NEXT || vkCode == VK_END || vkCode == VK_HOME ||
        vkCode == VK_INSERT || vkCode == VK_DELETE || vkCode == VK_DIVIDE || vkCode == VK_NUMLOCK) {
        scanCode |= 0x100;
    }
    
    wchar_t keyNameBuffer[256];
    int result = GetKeyNameTextW(scanCode << 16, keyNameBuffer, 256);
    
    if (result > 0) {
        return QString::fromWCharArray(keyNameBuffer);
    }
    
    switch (vkCode) {
        case VK_LBUTTON: return "Left Mouse Button";
        case VK_RBUTTON: return "Right Mouse Button";
        case VK_MBUTTON: return "Middle Mouse Button";
        case VK_XBUTTON1: return "X1 Mouse Button";
        case VK_XBUTTON2: return "X2 Mouse Button";
        case VK_LWIN: return "Left Windows";
        case VK_RWIN: return "Right Windows";
        case VK_APPS: return "Applications";
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        default: return QString("Unknown Key (VK_%1)").arg(vkCode);
    }
}

} // namespace KeyUtils
