#include "QtApplication.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QSettings>
    #include <QFileInfo>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

QString QtApplication::m_sSettingsPath;

std::unique_ptr<QSettings> QtApplication::getApplicationSettings()
{
    if (!m_sSettingsPath.isEmpty())
        return std::unique_ptr<QSettings>(new QSettings(m_sSettingsPath, QSettings::IniFormat));
    else
        return std::unique_ptr<QSettings>(new QSettings); // Uses QCoreApplication-properties organizationName, organizationDomain, applicationName.
}

QString QtApplication::getApplicationSettingsPath()
{
    return m_sSettingsPath;
}

bool QtApplication::createApplicationSettingsFile()
{
    if (QFileInfo::exists(getApplicationSettingsPath()))
        return true;
    QFile f(getApplicationSettingsPath());
    f.open(QIODevice::WriteOnly);
    f.close();
    return QFileInfo::exists(getApplicationSettingsPath());
}

} } // Module dfg::qt
