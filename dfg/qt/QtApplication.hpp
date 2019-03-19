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
    class DFG_CLASS_NAME(QtApplication)
    {
    public:
        static std::unique_ptr<QSettings> getApplicationSettings();
        static QString getApplicationSettingsPath();

        static QString m_sSettingsPath;
    };

}} // Module namespace

