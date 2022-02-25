#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include <memory>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QApplication>
DFG_END_INCLUDE_QT_HEADERS

class QSettings;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{
    class QtApplication
    {
    public:
        static std::unique_ptr<QSettings> getApplicationSettings();
        static QString getApplicationSettingsPath();
        static bool createApplicationSettingsFile(); // If no file exists at getApplicationSettingsPath(), creates file to that path. Returns true if file existed or was created, false otherwise.

        static QString m_sSettingsPath;
    };

}} // Module namespace

