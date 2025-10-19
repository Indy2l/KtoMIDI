#include "MainWindow.h"
#include "version.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSharedMemory>
#include <exception>

namespace {
    constexpr const char* APP_NAME = KTOMIDI_APP_NAME;
    constexpr const char* APP_VERSION = KTOMIDI_VERSION_STRING;
    constexpr const char* APP_DESCRIPTION = KTOMIDI_APP_DESCRIPTION;
    constexpr const char* ORGANIZATION_NAME = KTOMIDI_COMPANY_NAME;
    constexpr const char* ORGANIZATION_DOMAIN = "github.com/Indy2l/KtoMIDI";
    constexpr const char* SINGLE_INSTANCE_KEY = "KtoMIDI_SingleInstance";
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    QSharedMemory sharedMemory(SINGLE_INSTANCE_KEY);
    if (!sharedMemory.create(1)) {
        QMessageBox::warning(nullptr, APP_NAME,
            "KtoMIDI is already running.\n\n"
            "Only one instance can run at a time. "
            "Check your system tray for the running instance.");
        return 0;
    }
    
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    app.setApplicationDisplayName(APP_NAME);
    app.setOrganizationName(ORGANIZATION_NAME);
    app.setOrganizationDomain(ORGANIZATION_DOMAIN);
    
    QIcon appIcon(":/icons/KtoMIDI.ico");
    if (appIcon.isNull()) {
        qWarning() << "Could not load application icon from resources";
    } else {
        app.setWindowIcon(appIcon);
    }
    
    app.setQuitOnLastWindowClosed(false);
    
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available - background operation limited";
        QMessageBox::information(nullptr, APP_NAME, 
            "System tray is not available on this system.\n"
            "The application will function normally but cannot minimize to tray.");
    }
    
    QCommandLineParser parser;
    parser.setApplicationDescription(APP_DESCRIPTION);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("minimized", "Start minimized to system tray"));
    parser.process(app);
    
    try {
        MainWindow window;
        
        if (parser.isSet("minimized")) {
            qDebug() << "Starting minimized to system tray";
            window.hide();
        } else {
            window.show();
        }
        
        return app.exec();
        
    } catch (const std::exception& e) {
        qCritical() << "Fatal error:" << e.what();
        QMessageBox::critical(nullptr, APP_NAME, 
            QString("A fatal error occurred:\n%1\n\nThe application will now exit.").arg(e.what()));
        return -1;
    } catch (...) {
        qCritical() << "Unknown fatal error occurred";
        QMessageBox::critical(nullptr, APP_NAME, 
            "An unknown error occurred.\nThe application will now exit.");
        return -1;
    }
}