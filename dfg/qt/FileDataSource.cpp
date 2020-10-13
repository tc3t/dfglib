#include "CsvFileDataSource.hpp"
#include "qtBasic.hpp"
#include "../os.hpp"
#include "qtIncludeHelpers.hpp"
#include "connectHelper.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QFileSystemWatcher>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::FileDataSource::FileDataSource(const QString& sPath, QString sId)
{
    this->m_uniqueId = std::move(sId);
    this->m_bAreChangesSignaled = true;
    m_sPath = qStringToFileApi8Bit(sPath);
}

::DFG_MODULE_NS(qt)::FileDataSource::~FileDataSource() = default;

bool ::DFG_MODULE_NS(qt)::FileDataSource::privUpdateStatusAndAvailability()
{
    using namespace DFG_MODULE_NS(io);
    using namespace DFG_MODULE_NS(os);
    bool bAvailable = true;
    const auto bOldAvailability = isAvailable();
    if (!isPathFileAvailable(m_sPath, FileModeExists))
    {
        m_sStatusDescription = tr("No file exists in given path '%1'").arg(fileApi8BitToQString(m_sPath));
        m_spFileWatcher.reset(nullptr);
        bAvailable = false;
    }
    else if (!isPathFileAvailable(m_sPath, FileModeRead))
    {
        m_sStatusDescription = tr("File exists but is not readable in path '%1'").arg(fileApi8BitToQString(m_sPath));
        bAvailable = false;
    }

    if (bAvailable)
    {
        if (!m_spFileWatcher)
        {
            m_spFileWatcher.reset(new QFileSystemWatcher(QStringList() << fileApi8BitToQString(m_sPath), this));
            DFG_QT_VERIFY_CONNECT(connect(m_spFileWatcher.data(), &QFileSystemWatcher::fileChanged, this, &FileDataSource::onFileChanged));
        }
        if (bOldAvailability != bAvailable || m_columnIndexToColumnName.empty())
        {
            updateColumnInfoImpl();
            m_sStatusDescription.clear();
        }
    }

    setAvailability(bAvailable);
    return true;
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::statusDescriptionImpl() const -> String
{
    return m_sStatusDescription;
}

void ::DFG_MODULE_NS(qt)::FileDataSource::onFileChanged()
{
    m_columnIndexToColumnName.clear_noDealloc();
    privUpdateStatusAndAvailability();
    Q_EMIT sigChanged();
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::columnCount() const -> DataSourceIndex
{
    return (!m_columnIndexToColumnName.empty()) ? m_columnIndexToColumnName.backKey() + 1 : 0;
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::columnIndexes() const -> IndexList
{
    IndexList indexes;
    for (const auto& item : m_columnIndexToColumnName)
    {
        indexes.push_back(item.first);
    }
    return indexes;
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::columnIndexByName(const StringViewUtf8 sv) const -> DataSourceIndex
{
    return m_columnIndexToColumnName.keyByValue(sv, GraphDataSource::invalidIndex());
}

void ::DFG_MODULE_NS(qt)::FileDataSource::enable(bool b)
{
    DFG_UNUSED(b);
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::columnNames() const -> ColumnNameMap
{
    ColumnNameMap rv;
    for (const auto& item : m_columnIndexToColumnName)
    {
        rv[item.first] = QString::fromUtf8(item.second(m_columnIndexToColumnName).c_str().c_str());
    }
    return rv;
}

auto ::DFG_MODULE_NS(qt)::FileDataSource::columnDataTypes() const -> ColumnDataTypeMap
{
    return m_columnDataTypes;
}
