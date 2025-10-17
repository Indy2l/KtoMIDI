#pragma once

#define KTOMIDI_VERSION_MAJOR    1
#define KTOMIDI_VERSION_MINOR    0
#define KTOMIDI_VERSION_PATCH    0
#define KTOMIDI_VERSION_BUILD    0

#define KTOMIDI_VERSION_STRING   "1.0.0"
#define KTOMIDI_VERSION_FULL     "1.0.0.0"

#define KTOMIDI_APP_NAME         "KtoMIDI"
#define KTOMIDI_APP_DESCRIPTION  "Keyboard and HID to MIDI Converter"
#define KTOMIDI_COMPANY_NAME     "KtoMIDI Project"
#define KTOMIDI_COPYRIGHT        "Copyright (C) 2025 KtoMIDI Contributors"

#ifndef KTOMIDI_BUILD_DATE
#define KTOMIDI_BUILD_DATE       __DATE__
#endif

#ifndef KTOMIDI_BUILD_TIME
#define KTOMIDI_BUILD_TIME       __TIME__
#endif

#define KTOMIDI_FEATURE_KEYBOARD     1
#define KTOMIDI_FEATURE_HID_DEVICES  1
#define KTOMIDI_FEATURE_SYSTEM_TRAY  1
#define KTOMIDI_FEATURE_AUTO_CONNECT 1

#ifdef _WIN32
#define KTOMIDI_PLATFORM_WINDOWS 1
#else
#define KTOMIDI_PLATFORM_WINDOWS 0
#endif

#ifdef _DEBUG
#define KTOMIDI_DEBUG_BUILD 1
#else
#define KTOMIDI_DEBUG_BUILD 0
#endif

#define KTOMIDI_API
#define KTOMIDI_EXPORT

#define KTOMIDI_STRINGIFY(x) #x
#define KTOMIDI_TOSTRING(x) KTOMIDI_STRINGIFY(x)

#define KTOMIDI_VERSION_NUMBER \
    ((KTOMIDI_VERSION_MAJOR << 24) | \
     (KTOMIDI_VERSION_MINOR << 16) | \
     (KTOMIDI_VERSION_PATCH << 8)  | \
     (KTOMIDI_VERSION_BUILD))

#if KTOMIDI_DEBUG_BUILD
#define KTOMIDI_BUILD_CONFIG "Debug"
#else
#define KTOMIDI_BUILD_CONFIG "Release"
#endif

#define KTOMIDI_BUILD_INFO \
    KTOMIDI_APP_NAME " " KTOMIDI_VERSION_STRING " (" KTOMIDI_BUILD_CONFIG " build on " KTOMIDI_BUILD_DATE ")"

