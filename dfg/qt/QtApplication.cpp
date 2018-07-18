#include "QtApplication.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QSettings>
DFG_END_INCLUDE_QT_HEADERS

QString DFG_MODULE_NS(qt)::DFG_CLASS_NAME(QtApplication)::m_sSettingsPath;

std::unique_ptr<QSettings> DFG_MODULE_NS(qt)::DFG_CLASS_NAME(QtApplication)::getApplicationSettings()
{
    if (!m_sSettingsPath.isEmpty())
        return std::unique_ptr<QSettings>(new QSettings(m_sSettingsPath, QSettings::IniFormat));
    else
        return std::unique_ptr<QSettings>(new QSettings); // Uses QCoreApplication-properties organizationName, organizationDomain, applicationName.
}
