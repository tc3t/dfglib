#include "graphTools.hpp"

#include "connectHelper.hpp"
#include "widgetHelpers.hpp"
#include "ConsoleDisplay.hpp"
#include "ConsoleDisplay.cpp"
#include "JsonListWidget.hpp"
#include "PropertyHelper.hpp"
#include "stringConversions.hpp"

#include "qtBasic.hpp"

#include "../cont/IntervalSetSerialization.hpp"
#include "../cont/MapVector.hpp"
#include "../rangeIterator.hpp"

#include "../func/memFunc.hpp"
#include "../str/format_fmt.hpp"

#include "../alg.hpp"
#include "../cont/contAlg.hpp"
#include "../math.hpp"
#include "../scopedCaller.hpp"
#include "../str/strTo.hpp"
#include "../numeric/algNumeric.hpp"

#include "../time/timerCpu.hpp"

#include "../io.hpp"
#include "../io/BasicImStream.hpp"
#include "../io/DelimitedTextReader.hpp"

#include "../chartsAll.hpp"

#include "../build/compilerDetails.hpp"

#include "../iter/FunctionValueIterator.hpp"

#include <atomic>
#include <regex>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QPlainTextEdit>
    #include <QGridLayout>
    #include <QComboBox>
    #include <QLabel>
    #include <QMenu>
    #include <QMessageBox>
    #include <QPushButton>
    #include <QSplitter>
    #include <QCheckBox>
    #include <QDialog>
    #include <QTimer>
    #include <QElapsedTimer>
    #include <QGuiApplication>
    #include <QApplication>
    #include <QMainWindow>
    #include <QThread>

    #include <QListWidget>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QVariantMap>
    #include <QTextStream>
    #include <QLineEdit>

DFG_END_INCLUDE_QT_HEADERS

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include <boost/version.hpp>
    #if (BOOST_VERSION >= 107000) // Histogram is available since 1.70 (2019-04-12)
        #include <boost/histogram.hpp>
    #endif
    #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
        #include "qcustomplot/graphTools_qcustomplot.hpp"
    #endif
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

// Including before <boost/histogram.hpp> would cause compilation error on MSVC as this includes Shlwapi.h which for unknown reason breaks histogram
#include "CsvFileDataSource.hpp"
#include "SQLiteFileDataSource.hpp"
#include "NumberGeneratorDataSource.hpp"

using namespace DFG_MODULE_NS(charts)::fieldsIds;

auto ::DFG_MODULE_NS(qt)::chartBackendImplementationIdStr() -> QString
{
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
    return QString("QCustomPlot %1").arg(QCUSTOMPLOT_VERSION_STR);
#else
    return QString();
#endif
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) { namespace
{

    // Returns
    //      -defaultValue if not set
    //      -defaultValue + 1 if defined as row index.
    //      -GraphDataSource::invalidIndex() if set but not found.
    auto getChosenColumnIndex(GraphDataSource& dataSource,
                              const decltype(ChartObjectFieldIdStr_xSource)& id,
                              const GraphDefinitionEntry& defEntry,
                              const GraphDataSource::DataSourceIndex defaultValue)->GraphDataSource::DataSourceIndex;

    template <class ColRange_T, class IsRowSpecifier_T>
    void makeEffectiveColumnIndexes(GraphDataSource::DataSourceIndex& x, GraphDataSource::DataSourceIndex& y, GraphDataSource::DataSourceIndex& z, const ColRange_T& colRange, IsRowSpecifier_T isRowIndexSpecifier);

    ChartController* getControllerFromParents(QObject* pThis)
    {
        if (!pThis)
            return nullptr;
        auto pParent = pThis->parent();
        auto p = qobject_cast<ChartController*>(pParent);
        if (pParent && !p)
            p = qobject_cast<ChartController*>(pParent->parent());
        return p;
    }

    static void deleteLaterDeleter(GraphDataSource* p)
    {
        if (p) p->deleteLater();
    }

    static std::shared_ptr<NumberGeneratorDataSource> createNumberGeneratorSource(const GraphDataSourceId& id, const ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem& args)
    {
        using namespace ::DFG_MODULE_NS(math);
        if (args.valueCount() < 1 || args.valueCount() > 3)
            return nullptr;
        // First checking case (count, [first], [step]).
        {
            std::array<double, 3> numberArgs = { std::numeric_limits<double>::quiet_NaN(), 0, 1 };
            bool bSuccess = false;
            for (size_t i = 0, nCount = args.valueCount(); i < nCount; ++i)
            {
                numberArgs[i] = args.valueAs<double>(i, &bSuccess);
                if (!bSuccess)
                    break;
            }
            DataSourceIndex nCount = 0;
            if (isFloatConvertibleTo(numberArgs[0], &nCount) &&
                    isFinite(numberArgs[1]) &&
                    isFinite(numberArgs[2]))
                return std::shared_ptr<NumberGeneratorDataSource>(NumberGeneratorDataSource::createByCountFirstStep(id, nCount, numberArgs[1], numberArgs[2]).release(), deleteLaterDeleter);
        }

        // Getting here means that wasn't number form arguments, trying named arguments.
        using ParenthesisItem = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem;
        using StringView = ParenthesisItem::StringView;
        ::DFG_MODULE_NS(cont)::MapVectorSoA<StringView, double> argMap;
        for (const auto& arg : args)
        {
            auto namedArg = ParenthesisItem::fromStableView(arg);
            if (namedArg.key().empty() || namedArg.valueCount() != 1)
                return nullptr; // Got unrecognized arg, considering whole arg list invalid and returning nullptr.
            argMap[namedArg.key()] = namedArg.valueAs<double>(0);
        }
        /* Accepted combinations
            count, first, step
            count, first, last
            first, last, step
        */
        DataSourceIndex nCount = 0;
        const auto first = argMap.valueCopyOr(StringView(DFG_UTF8("first")), std::numeric_limits<double>::quiet_NaN());
        if (!isFinite(first))
            return nullptr;
        if (isFloatConvertibleTo(argMap.valueCopyOr(StringView(DFG_UTF8("count")), std::numeric_limits<double>::quiet_NaN()), &nCount))
        {
            // Getting here means that have first and count. Expecting either step or last
            auto step = argMap.valueCopyOr(StringView(DFG_UTF8("step")), std::numeric_limits<double>::quiet_NaN());
            if (isFinite(step)) // Case: Have count, first and step
                return std::shared_ptr<NumberGeneratorDataSource>(NumberGeneratorDataSource::createByCountFirstStep(id, nCount, first, step).release(), deleteLaterDeleter);

            const auto last = argMap.valueCopyOr(StringView(DFG_UTF8("last")), std::numeric_limits<double>::quiet_NaN());
            if (isFinite(last)) // Case: Have count, first and last
                return std::shared_ptr<NumberGeneratorDataSource>(NumberGeneratorDataSource::createByCountFirstLast(id, nCount, first, last).release(), deleteLaterDeleter);

            return nullptr;
        }
        else // Case: didn't have count, should have last and step
        {
            const auto step = argMap.valueCopyOr(StringView(DFG_UTF8("step")), std::numeric_limits<double>::quiet_NaN());
            const auto last = argMap.valueCopyOr(StringView(DFG_UTF8("last")), std::numeric_limits<double>::quiet_NaN());
            if (isFinite(step) && isFinite(last))
                return std::shared_ptr<NumberGeneratorDataSource>(NumberGeneratorDataSource::createByfirstLastStep(id, first, last, step).release(), deleteLaterDeleter);
            return nullptr;
        }
    }

    static auto tryCreateOnDemandDataSource(const GraphDataSourceId& id, DataSourceContainer& dataSources) -> DataSourceContainer::iterator
    {
        const auto sId = qStringToStringUtf8(id);
        ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem item(sId);
        std::shared_ptr<GraphDataSource> spNewSource;
        if (item.key() == DFG_UTF8("csv_file"))
            spNewSource = std::shared_ptr<CsvFileDataSource>(new CsvFileDataSource(viewToQString(item.value(0)), id), deleteLaterDeleter);
        else if (item.key() == DFG_UTF8("sqlite_file"))
            spNewSource = std::shared_ptr<SQLiteFileDataSource>(new SQLiteFileDataSource(viewToQString(item.value(1)), viewToQString(item.value(0)), id), deleteLaterDeleter);
        else if (item.key() == DFG_UTF8("number_generator_even_step"))
            spNewSource = createNumberGeneratorSource(id, item);

        if (spNewSource)
        {
            dataSources.m_sources.push_back(std::move(spNewSource));
            return dataSources.end() - 1;
        }
        else
            return dataSources.end();
    }

} } } // dfg:::qt::<unnamed>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

namespace DFG_DETAIL_NS
{
    ConsoleLogHandle::~ConsoleLogHandle() = default;

    void ConsoleLogHandle::setHandler(HandlerT handler)
    {
        m_handler = std::move(handler);
        privSetEffectiveLevel();
    }

    void ConsoleLogHandle::setDesiredLevel(ConsoleLogLevel newLevel)
    {
        m_desiredLevel = newLevel;
        privSetEffectiveLevel();
    }

    void ConsoleLogHandle::log(const char* psz, const ConsoleLogLevel msgLogLevel)
    {
        if (m_handler)
            m_handler(psz, msgLogLevel);
    }

    void ConsoleLogHandle::log(const QString& s, const ConsoleLogLevel msgLogLevel)
    {
        if (m_handler)
            m_handler(s.toUtf8(), msgLogLevel);
    }

    void ConsoleLogHandle::privSetEffectiveLevel()
    {
        if (m_handler)
            m_effectiveLevel = m_desiredLevel;
        else
            m_effectiveLevel = ConsoleLogLevel::none;
    }

    ConsoleLogLevel ConsoleLogHandle::effectiveLevel() const { return m_effectiveLevel; }

    ConsoleLogHandle gConsoleLogHandle;

    ::DFG_MODULE_NS(charts)::ChartEntryOperationManager& operationManager()
    {
        static auto operationManager = []()
            {
                ::DFG_MODULE_NS(charts)::ChartEntryOperationManager manager;
                manager.setStringToDoubleConverter([](const StringViewSzUtf8 sv) { return GraphDataSource::cellStringToDouble(sv, 0, nullptr); });
                return manager;
            }();
        return operationManager;
    }

    QString formatOperationErrorsForUserVisibleText(const ::DFG_MODULE_NS(charts)::ChartEntryOperation& op)
    {
        const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
        return tr("error mask = 0x%1").arg(op.m_errors.toNumber(), 0, 16);
    }

    static ConsoleDisplayEntryType consoleLogLevelToEntryType(const ConsoleLogLevel logLevel)
    {
        switch (logLevel)
        {
            case ConsoleLogLevel::error:   return ConsoleDisplayEntryType::error;
            case ConsoleLogLevel::warning: return ConsoleDisplayEntryType::warning;
            default:                       return ConsoleDisplayEntryType::generic;
        }
    }

} // namespace DFG_DETAIL_NS

void ChartController::refresh()
{
    if (m_bRefreshInProgress)
        return; // refreshImpl() already in progress; not calling it.

    // Implementation must reset this flag once refresh is done
    m_bRefreshInProgress = true;

    refreshImpl();
}

bool ChartController::refreshTerminate()
{
    if (!m_bRefreshInProgress)
        return false;
    return refreshTerminateImpl();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   DefaultNameCreator
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DefaultNameCreator
{
public:
    // pszType must point to valid string for the lifetime of 'this'.
    DefaultNameCreator(const char* pszType, const int nIndex, const QString& sXname = QString(), const QString& sYname = QString());

    DefaultNameCreator(StringUtf8 sName);

    StringUtf8 operator()() const;

    StringUtf8 m_sName;
    const char* m_pszType = nullptr;
    int m_nIndex = -1;
    QString m_sXname;
    QString m_sYname;
};

DefaultNameCreator::DefaultNameCreator(StringUtf8 sName)
    : m_sName(std::move(sName))
{
}

DefaultNameCreator::DefaultNameCreator(const char* pszType, const int nIndex, const QString& sXname, const QString& sYname)
    : m_pszType(pszType)
    , m_nIndex(nIndex)
    , m_sXname(sXname)
    , m_sYname(sYname)
{
}

auto DefaultNameCreator::operator()() const -> StringUtf8
{
    if (!m_sName.empty())
        return m_sName;
    if (!m_sXname.isEmpty() || !m_sYname.isEmpty())
        return DFG_ROOT_NS::StringUtf8::fromRawString(DFG_ROOT_NS::format_fmt("{} {} ('{}', '{}')", m_pszType, m_nIndex, m_sXname.toUtf8().data(), m_sYname.toUtf8().data()));
    else
        return DFG_ROOT_NS::StringUtf8::fromRawString(DFG_ROOT_NS::format_fmt("{} {}", m_pszType, m_nIndex));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   DataSourceContainer
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

auto DataSourceContainer::idListAsString() const -> QString
{
    QString s;
    QTextStream strm(&s);
    dfg::io::writeDelimited(strm, this->m_sources, QLatin1String(", "), [](QTextStream& strm, const std::shared_ptr<GraphDataSource>& sp)
    {
        if (sp)
            strm << sp->uniqueId();
    });
    return s;
}

auto DataSourceContainer::findById(const GraphDataSourceId& id) -> iterator
{
    return std::find_if(m_sources.begin(), m_sources.end(), [&](const std::shared_ptr<GraphDataSource>& spDs)
        {
            return (spDs && spDs->uniqueId() == id);
        });
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDataSource
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


double ::DFG_MODULE_NS(qt)::GraphDataSource::dateToDouble(QDateTime&& dt)
{
    return DFG_DETAIL_NS::tableCellDateToDouble(std::move(dt));
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::timeToDouble(const QTime& t)
{
    return DFG_DETAIL_NS::tableCellTimeToDouble(t);
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::stringToDouble(const QString& s)
{
    return ::DFG_MODULE_NS(qt)::stringToDouble(s);
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::stringToDouble(const StringViewSzC& sv)
{
    return ::DFG_MODULE_NS(qt)::stringToDouble(sv);
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::cellStringToDouble(const StringViewSzUtf8& sv)
{
    return cellStringToDouble(sv, GraphDataSource::invalidIndex(), nullptr);
}

// Return value in case of invalid input as GIGO, in most cases returns NaN. Also in case of invalid input typeMap's value at nCol is unspecified.
double ::DFG_MODULE_NS(qt)::GraphDataSource::cellStringToDouble(const StringViewSzUtf8& svUtf8, const DataSourceIndex nCol, ColumnDataTypeMap* pTypeMap)
{
    ChartDataType dataType;
    const auto rv = ::DFG_MODULE_NS(qt)::tableCellStringToDouble(svUtf8, &dataType);

    if (pTypeMap && dataType != ChartDataType::unknown)
    {
        auto insertRv = pTypeMap->insert(nCol, dataType);
        if (!insertRv.second) // Already existed?
            insertRv.first->second.setIfExpands(dataType);
    }
    return rv;
}

auto ::DFG_MODULE_NS(qt)::GraphDataSource::columnDataType(const DataSourceIndex nCol) const -> ChartDataType
{
    const auto colTypes = columnDataTypes();
    auto iter = colTypes.find(nCol);
    return (iter != colTypes.end()) ? iter->second : ChartDataType();
}

bool ::DFG_MODULE_NS(qt)::GraphDataSource::isSafeToQueryDataFromCallingThread() const
{
    return isSafeToQueryDataFromThread(QThread::currentThread());
}

bool ::DFG_MODULE_NS(qt)::GraphDataSource::isSafeToQueryDataFromThreadImpl(const QThread* pThread) const
{
    return this->thread() == pThread;
}

void ::DFG_MODULE_NS(qt)::GraphDataSource::fetchColumnNumberData(GraphDataSourceDataPipe&& pipe, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails)
{
    // Default implementation using forEachElement_byColumn()
    forEachElement_byColumn(nColumn, queryDetails, [&](const SourceDataSpan& inputSpan)
    {
        double* pRows = nullptr;
        double* pDoubles = nullptr;
        pipe.getFillBuffers(inputSpan.size(),
                            (!inputSpan.rows().empty()) ? &pRows : nullptr,
                            (!inputSpan.doubles().empty()) ? &pDoubles : nullptr
                            );
        if (pRows)
            std::copy(inputSpan.rows().begin(), inputSpan.rows().end(), pRows);
        if (pDoubles)
            std::copy(inputSpan.doubles().begin(), inputSpan.doubles().end(), pDoubles);
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDefinitionEntry
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: most/all of this should be in AbstractChartControlItem. For now mostly here due to the readily available json-tools in Qt.
class GraphDefinitionEntry : public ::DFG_MODULE_NS(charts)::AbstractChartControlItem
{
public:
    using Operation = ::DFG_MODULE_NS(charts)::ChartEntryOperation;

    static GraphDefinitionEntry xyGraph(const QString& sColumnX, const QString& sColumnY)
    {
        QVariantMap keyVals;
        keyVals.insert(ChartObjectFieldIdStr_type, ChartObjectChartTypeStr_xy);
        keyVals.insert(ChartObjectFieldIdStr_lineStyle, ChartObjectLineStyleStr_basic);
        keyVals.insert(ChartObjectFieldIdStr_pointStyle, ChartObjectPointStyleStr_basic);
        DFG_UNUSED(sColumnX);
        DFG_UNUSED(sColumnY);
        GraphDefinitionEntry rv;
        rv.m_items = QJsonDocument::fromVariant(keyVals);
        return rv;
    }

    static GraphDefinitionEntry fromText(const QString& sJson, const int nIndex);

    GraphDefinitionEntry() {}

    QString toText() const
    {
        return m_items.toJson(QJsonDocument::Compact);
    }

    bool isEnabled() const;

    template <class Func_T>
    void doForLineStyleIfPresent(Func_T&& func) const
    {
        doForFieldIfPresent(ChartObjectFieldIdStr_lineStyle, std::forward<Func_T>(func));
    }

    template <class Func_T>
    void doForPointStyleIfPresent(Func_T&& func) const
    {
        doForFieldIfPresent(ChartObjectFieldIdStr_pointStyle, std::forward<Func_T>(func));
    }

    GraphDataSourceId sourceId(const GraphDataSourceId& sDefault) const;

    int index() const { return m_nContainerIndex; }

    bool isType(FieldIdStrViewInputParam fieldId) const;

    void applyOperations(::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData) const;

    static QString tr(const char* psz);

    // Implements logging of notifications related to 'this' entry.
    // Note that log level is not checked -> every call will result to logging. Caller should use isLoggingAllowedForLevel() for filtering.
    static void log(LogLevel logLevel, int nIndex, const QString& sMsg);
    void log(LogLevel logLevel, const QString& sMsg) const;

    StringViewOrOwner graphTypeStr() const override;

private:
    QJsonValue getField(FieldIdStrViewInputParam fieldId) const; // fieldId must be a ChartObjectFieldIdStr_

    std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const override;
    void forEachPropertyIdImpl(std::function<void(StringView svId)>) const override;

    template <class Func_T>
    void doForFieldIfPresent(FieldIdStrViewInputParam id, Func_T&& func) const;

    int m_nContainerIndex = -1;
    QJsonDocument m_items;
    ::DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, Operation> m_operationMap;
}; // class GraphDefinitionEntry

auto GraphDefinitionEntry::fromText(const QString& sJson, const int nIndex) -> GraphDefinitionEntry
{
    GraphDefinitionEntry rv;
    QJsonParseError parseError;
    rv.m_items = QJsonDocument::fromJson(sJson.toUtf8(), &parseError);
    if (rv.m_items.isNull()) // Parsing failed?
    {
        // If parsing failed, creating a disabled entry that has field error_string describing the parse error.
        QString sJsonError = parseError.errorString();
        sJsonError.replace('"', '\'');
        const auto nOffset = parseError.offset;
        const QString sError = QString("parse error '%1' at offset %2").arg(sJsonError).arg(nOffset);
        const QString sReplacementJson = QString(R"( { "enabled":false, "%1":"%2" } )").arg(untypedViewToQStringAsUtf8(ChartObjectFieldIdStr_errorString), sError);
        rv.m_items = QJsonDocument::fromJson(sReplacementJson.toUtf8(), &parseError);
        DFG_ASSERT(!rv.m_items.isNull());
    }
    rv.m_nContainerIndex = nIndex;
    bool bHasLogLevelField = false;
    const auto sLogLevel = rv.fieldValueStr(ChartObjectFieldIdStr_logLevel, &bHasLogLevelField);
    if (bHasLogLevelField)
    {
        bool bValidLogLevel = false;
        rv.logLevel(sLogLevel, &bValidLogLevel);
        if (!bValidLogLevel && rv.isLoggingAllowedForLevel(LogLevel::error))
        {
            rv.log(LogLevel::error, tr("Invalid log level '%1', using default log level").arg(viewToQString(sLogLevel)));
            rv.logLevel(DFG_DETAIL_NS::gConsoleLogHandle.effectiveLevel());
        }
    }
    else // Case: entry does not have log_level field. Setting level to global default.
        rv.logLevel(DFG_DETAIL_NS::gConsoleLogHandle.effectiveLevel());

    // Reading operations
    {
        auto& manager = DFG_DETAIL_NS::operationManager();
        rv.forEachPropertyIdStartingWith(ChartObjectFieldIdStr_operation, [&](const StringViewUtf8& svKey, const StringViewUtf8 svOperationOrderTag)
        {
            const auto sOperationDef = rv.fieldValueStr(svKey.asUntypedView());
            auto op = manager.createOperation(sOperationDef);
            if (!op)
            {
                if (rv.isLoggingAllowedForLevel(LogLevel::warning))
                    rv.log(LogLevel::warning, tr("Unable to create '%1: %2'").arg(viewToQString(svKey), viewToQString(sOperationDef)));
            }
            else
            {
                op.m_sDefinition = sOperationDef;
                rv.m_operationMap[svOperationOrderTag.toString()] = std::move(op);
            }
        });
    }

    return rv;
}

QString GraphDefinitionEntry::tr(const char* psz)
{
    return QCoreApplication::tr(psz);
}

void GraphDefinitionEntry::log(const LogLevel logLevel, const int nIndex, const QString& sMsgBody)
{
    QString s = tr("Entry %1: %2").arg(nIndex).arg(sMsgBody);
    DFG_QT_CHART_CONSOLE_LOG_NO_LEVEL_CHECK(logLevel, s);
}

void GraphDefinitionEntry::log(const LogLevel logLevel, const QString& sMsgBody) const
{
    log(logLevel, this->m_nContainerIndex, sMsgBody);
}

QJsonValue GraphDefinitionEntry::getField(FieldIdStrViewInputParam fieldId) const
{
    return m_items.object().value(QLatin1String(fieldId.data(), static_cast<int>(fieldId.size())));
}

bool GraphDefinitionEntry::isEnabled() const
{
    return getField(ChartObjectFieldIdStr_enabled).toBool(true); // If field is not present, defaulting to 'enabled'.
}

auto GraphDefinitionEntry::graphTypeStr() const -> StringViewOrOwner
{
    return StringViewOrOwner::makeOwned(std::string(getField(ChartObjectFieldIdStr_type).toString().toUtf8()));
}

template <class Func_T>
void GraphDefinitionEntry::doForFieldIfPresent(FieldIdStrViewInputParam id, Func_T&& func) const
{
    const auto val = getField(id);
    if (!val.isNull() && !val.isUndefined())
        func(val.toString().toUtf8());
}

auto GraphDefinitionEntry::fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const -> std::pair<bool, String>
{
    const auto val = getField(fieldId);
    return (!val.isNull() && !val.isUndefined()) ? std::make_pair(true, String(SzPtrUtf8(val.toVariant().toString().toUtf8()))) : std::make_pair(false, String());
}

void GraphDefinitionEntry::forEachPropertyIdImpl(std::function<void(StringView svId)> func) const
{
    if (!func)
        return;
    const auto keys = m_items.object().keys();
    for (const auto& sKey : keys)
    {
        func(SzPtrUtf8(sKey.toUtf8()));
    }
}

bool GraphDefinitionEntry::isType(FieldIdStrViewInputParam fieldId) const
{
    return getField(ChartObjectFieldIdStr_type).toString().toUtf8().data() == fieldId;
}

auto GraphDefinitionEntry::sourceId(const GraphDataSourceId& sDefault) const -> GraphDataSourceId
{
    const auto val = getField(ChartObjectFieldIdStr_dataSource);
    if (!val.isNull() && !val.isUndefined())
        return val.toString();
    else
        return sDefault;
}

void GraphDefinitionEntry::applyOperations(::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData) const
{
    for (const auto& kv : this->m_operationMap)
    {
        auto opCopy = kv.second;
        if (isLoggingAllowedForLevel(LogLevel::debug))
            log(LogLevel::debug, tr("Running operation %1: %2").arg(viewToQString(kv.first), viewToQString(opCopy.m_sDefinition)));
        opCopy(pipeData);
        if (opCopy.hasErrors())
        {
            if (isLoggingAllowedForLevel(LogLevel::warning))
                this->log(LogLevel::warning, tr("operation '%1' encountered errors, %2").arg(viewToQString(opCopy.m_sDefinition), DFG_DETAIL_NS::formatOperationErrorsForUserVisibleText(opCopy)));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   ChartDataCache
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class TableSelectionCacheItem : public QObject
{
public:
    using IndexT  = GraphDataSource::DataSourceIndex;
    using StringT = ::DFG_ROOT_NS::StringUtf8;
    using ValueVectorD        = ::DFG_MODULE_NS(charts)::ValueVectorD;
    using RowToValueMap       = ::DFG_MODULE_NS(cont)::MapVectorSoA<double, double, ValueVectorD, ValueVectorD>;
    using RowToStringMap      = ::DFG_MODULE_NS(cont)::MapVectorSoA<double, StringT, ValueVectorD>;
    using ColumnToValuesMap   = ::DFG_MODULE_NS(cont)::MapVectorSoA<IndexT, RowToValueMap>;
    using ColumnToStringsMap  = ::DFG_MODULE_NS(cont)::MapVectorSoA<IndexT, RowToStringMap>;
    using DataPipeForTableCache = GraphDataSourceDataPipe_MapVectorSoADoubleValueVector;

    std::vector<std::reference_wrapper<const RowToValueMap>> columnDatas() const;

    const RowToValueMap* columnDataByIndex(IndexT nColumnIndex);
    const RowToStringMap* columnStringsByIndex(IndexT nColumnIndex);

    IndexT columnCount() const;

    IndexT firstColumnIndex() const; // GraphDataSource::invalidIndex() if not defined
    IndexT lastColumnIndex() const; // GraphDataSource::invalidIndex() if not defined

    decltype(ColumnToValuesMap().keyRange()) columnRange() const;

    bool isValid() const { return m_bIsValid; }

    RowToValueMap releaseOrCopy(const RowToValueMap* pId);

    bool hasColumnIndex(const IndexT nCol) const;

    ChartDataType columnDataType(const RowToValueMap* pColumn) const;
    QString columnName(const RowToValueMap* pColumn) const;
    QString columnName(DataSourceIndex nColumn) const;

    IndexT columnToIndex(const RowToValueMap* pColumn) const;

    bool storeColumnFromSource(GraphDataSource& source, const DataSourceIndex nColumn);
    bool storeColumnFromSource_strings(GraphDataSource& source, const DataSourceIndex nColumn);

    // CacheItem is volatile if it doesn't have mechanism to know when it's source has changed.
    bool isVolatileCache() const;

    void onDataSourceChanged();

private:
    template <class Map_T, class Inserter_T>
    bool storeColumnFromSourceImpl(Map_T& mapIndexToStorage, GraphDataSource& source, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails, Inserter_T inserter);

public:

    ColumnToValuesMap m_colToValuesMap;
    ColumnToStringsMap m_colToStringsMap;
    GraphDataSource::ColumnDataTypeMap m_columnTypes;
    GraphDataSource::ColumnNameMap m_columnNames;
    bool m_bIsValid = false;
    QPointer<GraphDataSource> m_spSource;
};

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnDatas() const -> std::vector<std::reference_wrapper<const RowToValueMap>>
{
    std::vector<std::reference_wrapper<const RowToValueMap>> datas;
    for (const auto& d : m_colToValuesMap)
        datas.push_back(d.second);
    return datas;
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnCount() const -> IndexT
{
    return m_colToValuesMap.size();
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnRange() const -> decltype(m_colToValuesMap.keyRange())
{
    return m_colToValuesMap.keyRange();
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnDataByIndex(IndexT nColumnIndex) -> const RowToValueMap*
{
    auto iter = m_colToValuesMap.find(nColumnIndex);
    return (iter != m_colToValuesMap.end()) ? &iter->second : nullptr;
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnStringsByIndex(IndexT nColumnIndex) -> const RowToStringMap*
{
    auto iter = m_colToStringsMap.find(nColumnIndex);
    return (iter != m_colToStringsMap.end()) ? &iter->second : nullptr;
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::firstColumnIndex() const -> IndexT
{
    return (!m_colToValuesMap.empty()) ? m_colToValuesMap.frontKey() : GraphDataSource::invalidIndex();
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::lastColumnIndex() const -> IndexT
{
    return (!m_colToValuesMap.empty()) ? m_colToValuesMap.backKey() : GraphDataSource::invalidIndex();
}

bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::hasColumnIndex(const IndexT nCol) const
{
    return m_colToValuesMap.hasKey(nCol);
}

void DFG_MODULE_NS(qt)::TableSelectionCacheItem::onDataSourceChanged()
{
    // Source has changes, simply marking cache as invalid so that it gets recreated.
    this->m_bIsValid = false;
}

::DFG_MODULE_NS(qt)::GraphDataSourceDataPipe_MapVectorSoADoubleValueVector::GraphDataSourceDataPipe_MapVectorSoADoubleValueVector(TableSelectionCacheItem::RowToValueMap* p)
    : m_pData(p)
{}

void ::DFG_MODULE_NS(qt)::GraphDataSourceDataPipe_MapVectorSoADoubleValueVector::getFillBuffers(const DataSourceIndex nCount, double** ppRows, double** ppValues)
{
    DFG_ASSERT_WITH_MSG(ppRows != nullptr, "Development assert: null ppRows in getFillBuffers() might not work correctly"); // There might be some unwanted consequences if all keys have value 0.
    if (ppRows)
        *ppRows = nullptr;
    if (ppValues)
        *ppValues = nullptr;
    if (!m_pData)
        return;
    const auto nOldSize = m_pData->size();
    const auto nNewSize = saturateAdd<size_t>(m_pData->size(), nCount);
    m_pData->m_keyStorage.resize(nNewSize);
    m_pData->m_valueStorage.resize(nNewSize);
    
    if (ppRows)
        *ppRows = makeRange(m_pData->beginKey() + nOldSize, m_pData->beginKey() + nOldSize + 1).beginAsPointer(); // Using makeRange().beginAsPointer() to make sure that keys are in contiguous range.
    if (ppValues)
        *ppValues = makeRange(m_pData->beginValue() + nOldSize, m_pData->beginValue() + nOldSize + 1).beginAsPointer(); // Using makeRange().beginAsPointer() to make sure that values are in contiguous range.
}

namespace DFG_DETAIL_NS
{
    template <class T>
    bool handleStoreColumnDirectFetch(const T&, GraphDataSource&, const DataSourceIndex, const DataQueryDetails&)
    {
        return false;
    }

    bool handleStoreColumnDirectFetch(TableSelectionCacheItem::RowToValueMap& destValues, GraphDataSource& source, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails)
    {
        if (!queryDetails.areStringsRequested())
        {
            DFG_ASSERT(queryDetails.areRowsRequested()); // If there are no rows, destValues.keys() are bogus and data is expected to malfunction in graphs.
            source.fetchColumnNumberData(TableSelectionCacheItem::DataPipeForTableCache(&destValues), nColumn, queryDetails);
            return true;
        }
        else
            return false;
    }
} // namespace DFG_DETAIL_NS


template <class Map_T, class Insert_T>
bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::storeColumnFromSourceImpl(Map_T& mapIndexToStorage, GraphDataSource& source, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails, Insert_T inserter)
{
    if (m_spSource && m_spSource != &source)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Internal error: cache item source changed, was '%1', now using '%2'").arg(m_spSource->uniqueId(), source.uniqueId()));
        mapIndexToStorage.clear();
    }
    if (m_spSource != &source)
    {
        m_spSource = &source;
        DFG_QT_VERIFY_CONNECT(QObject::connect(&source, &GraphDataSource::sigChanged, this, &TableSelectionCacheItem::onDataSourceChanged));
    }
    auto insertRv = mapIndexToStorage.insert(nColumn, typename Map_T::mapped_type());
    if (!insertRv.second)
        return true; // Column was already present; since currently caching stores whole column, it should already have everything ready so nothing left to do.

    insertRv.first->second.setSorting(false); // Disabling sorting while adding
    auto& destValues = insertRv.first->second;
    if (!DFG_DETAIL_NS::handleStoreColumnDirectFetch(destValues, source, nColumn, queryDetails))
    {
        // Direct fetch failed/not avaiable, fallback to forEachElement_byColumn().
        source.forEachElement_byColumn(nColumn, queryDetails, [&](const SourceDataSpan& sourceData)
        {
            inserter(destValues, sourceData);
        });
    }
    
    destValues.setSorting(true, std::is_sorted(destValues.beginKey(), destValues.endKey()));
    m_columnTypes = source.columnDataTypes();
    m_columnNames = source.columnNames();
    this->m_bIsValid = true;
    return true;
}


bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::storeColumnFromSource(GraphDataSource& source, const DataSourceIndex nColumn)
{
    const auto inserter = [&](RowToValueMap& values, const SourceDataSpan& sourceData)
    {
        const auto& doubleRange = sourceData.doubles();
        if (sourceData.rows().empty() && !doubleRange.empty())
        {
            // If data has doubles but not rows, generating iota rows.
            auto iter = ::DFG_MODULE_NS(iter)::makeFunctionValueIterator(size_t(values.size()), [](size_t i) { return i; });
            values.pushBackToUnsorted(makeRange(iter, iter + doubleRange.size()), doubleRange);
        }
        else
            values.pushBackToUnsorted(sourceData.rows(), doubleRange);
    };
    
    return storeColumnFromSourceImpl(m_colToValuesMap, source, nColumn, DataQueryDetails(DataQueryDetails::DataMaskRowsAndNumerics), inserter);
}

bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::storeColumnFromSource_strings(GraphDataSource& source, const DataSourceIndex nColumn)
{
    const auto inserter = [&](RowToStringMap& rowToStringMap, const SourceDataSpan& sourceData)
    {
        const auto rowRange = sourceData.rows();
        const auto stringViews = sourceData.stringViews();
        const auto rowIdentityFunc = [](const double row) { return row; };
        if (!stringViews.empty())
        {
            rowToStringMap.pushBackToUnsorted(rowRange, rowIdentityFunc,
                                              stringViews,
                                              [](const StringViewUtf8& sv) { return StringUtf8::fromRawString(sv.beginRaw(), sv.endRaw()); });
            return;
        }
        const auto values = sourceData.doubles();
        if (!values.empty())
        {
            rowToStringMap.pushBackToUnsorted(rowRange, rowIdentityFunc,
                                              values,
                                              [](const double d) { return ::DFG_MODULE_NS(str)::floatingPointToStr<StringUtf8>(d); });
            return;
        }
    };
    return storeColumnFromSourceImpl(m_colToStringsMap, source, nColumn, DataQueryDetails(DataQueryDetails::DataMaskRowsAndStrings), inserter);
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::releaseOrCopy(const RowToValueMap* pId) -> RowToValueMap
{
    auto iter = std::find_if(m_colToValuesMap.begin(), m_colToValuesMap.end(), [=](const auto& v)
    {
        return &v.second == pId;
    });
    if (iter != m_colToValuesMap.end())
    {
        if (dynamic_cast<const NumberGeneratorDataSource*>(this->m_spSource.data()) == nullptr)
            return iter->second; // As there's no mechanism to verify if cacheItem is to be used by someone else, always copying.
        else // Case: source is number generator. Since generating is cheap, always releasing the content. This effetively means that caching is not used for NumberGenerator even though values are still fetched through cache object.
        {
            this->m_bIsValid = false; // Data gets moved out so marking this cache item invalid.
            return std::move(iter->second);
        }
    }
    else
        return RowToValueMap();
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnToIndex(const RowToValueMap* pColumn) const -> IndexT
{
    auto iter = std::find_if(m_colToValuesMap.begin(), m_colToValuesMap.end(), [=](const auto& v)
    {
        return &v.second == pColumn;
    });
    return (iter != m_colToValuesMap.end()) ? iter->first : GraphDataSource::invalidIndex();
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnDataType(const RowToValueMap* pColumn) const -> ChartDataType
{
    const auto nCol = columnToIndex(pColumn);
    auto iter = m_columnTypes.find(nCol);
    return (iter != m_columnTypes.end()) ? iter->second : ChartDataType(ChartDataType::unknown);
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnName(const RowToValueMap* pColumn) const -> QString
{
    return columnName(columnToIndex(pColumn));
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::columnName(const DataSourceIndex nCol) const -> QString
{
    auto iter = m_columnNames.find(nCol);
    return (iter != m_columnNames.end()) ? iter->second : QString();
}

bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::isVolatileCache() const
{
    auto pSource = m_spSource.data();
    return (!pSource || !pSource->hasChangeSignaling());
}


/*
 * ChartDataCache
 * --------------
 *
 * Implements caching for fetching data from DataSource for ChartObjects.
 *
 * Why caching instead of querying directly?
 *      -At this level there's little information about DataSource and what it actually does when querying data (e.g. disk access perhaps?)
 *          -> avoiding redundant queries is reasonable.
 *      -Generic dataSource does not have optimized queries for specific type of chart objects
 *          -> by fetching data once and storing it in cache in specific format allows efficient handling of similar ChartObjects.
 *
 * Current implementation in the context of dfgQtTableEditor and CsvTableView selection source:
 *  1. User changes selection
 *  2. SelectionAnalyzerForGraphing notices changed selection and stores selection in thread-safe manner.
 *  3. Changed selection leads to recreation of ChartObjects due to data changed -signal:
 *      a. ChartObjects do not directly query data from data source, but instead they ask data from object of this class (ChartDataCache)
 *      b. ChartObject definition defines a cacheKey so that items with identical cacheKeys can use the same fetched cache data.
 *          -Examples:
 *              -Two xySeries with only difference being panel in which they are drawn shall have identical cacheKey so they can be constructed from the same cache-data.
 *              -Two xySeries otherwise identical, but other has x_rows-filter
 *                  -This is implementation dependent: if caching stores the whole column in any case, then cacheKey can be identical, otherwise not.
 */
class ChartDataCache
{
public:
    using CacheEntryKey = QString;

    // Cached types
    using TableSelectionOptional = std::shared_ptr<TableSelectionCacheItem>;
    using SingleColumnDoubleValuesOptional = GraphDataSource::SingleColumnDoubleValuesOptional;
    using AbstractChartControlItem = ::DFG_MODULE_NS(charts)::AbstractChartControlItem;

    // Maps
    using CacheKeyToTableSelectionaMap = std::map<CacheEntryKey, TableSelectionOptional>;
    using CacheKeyToColumnDoubleValuesMap = std::map<CacheEntryKey, SingleColumnDoubleValuesOptional>;

    CacheEntryKey cacheKey(const GraphDataSource& source, const AbstractChartControlItem& defEntry) const;

    // Removes caches that have been invalidated.
    void removeInvalidCaches();

    /**
     *   Returns TableSelectionOptional, which is effectively an optional list of column data: map columnIndex -> column data.
     *      -The list may have more than requested number of columns. For xy-type, column index of x is placed in rColumnIndexes[0] and y in rColumnIndexes[1] respectively.
     *      -rRowFlag-array receives isColumnRowIndex() flags.
     */
    TableSelectionOptional getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry, std::array<DataSourceIndex, 3>& rColumnIndexes, std::array<bool, 3>& rRowFlags);

    CacheKeyToTableSelectionaMap m_tableSelectionDatas;
}; // ChartDataCache


auto DFG_MODULE_NS(qt)::ChartDataCache::getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry, std::array<DataSourceIndex, 3>& rColumnIndexes, std::array<bool, 3>& rRowFlags) -> TableSelectionOptional
{
    std::fill(rColumnIndexes.begin(), rColumnIndexes.end(), GraphDataSource::invalidIndex());
    std::fill(rRowFlags.begin(), rRowFlags.end(), false);
    const auto columns = source.columnIndexes();
    if (columns.empty())
        return TableSelectionOptional();
    auto key = this->cacheKey(source, defEntry);
    auto iter = m_tableSelectionDatas.find(key);
    if (iter == m_tableSelectionDatas.end())
        iter = m_tableSelectionDatas.insert(std::make_pair(key, std::make_shared<TableSelectionCacheItem>())).first;
    else if (iter->second && !iter->second->isValid())
        iter->second = std::make_shared<TableSelectionCacheItem>(); // Cache item existed, but was invalid -> creating a new one.

    auto& rCacheItem = *iter->second;

    const auto nLastColumnIndex = columns.back();
    const auto nDefaultValue = nLastColumnIndex + 1;
    const auto nRowColumnSpecifier = nLastColumnIndex + 2;

    auto& xColumnIndex = rColumnIndexes[0];
    auto& yColumnIndex = rColumnIndexes[1];
    auto& zColumnIndex = rColumnIndexes[2];
    xColumnIndex = getChosenColumnIndex(source, ChartObjectFieldIdStr_xSource, defEntry, nDefaultValue);
    yColumnIndex = getChosenColumnIndex(source, ChartObjectFieldIdStr_ySource, defEntry, nDefaultValue);
    zColumnIndex = getChosenColumnIndex(source, ChartObjectFieldIdStr_zSource, defEntry, nDefaultValue);

    const auto isRowIndexSpecifier = [=](const DataSourceIndex i) { return i == nRowColumnSpecifier; };

    if (xColumnIndex == GraphDataSource::invalidIndex() || yColumnIndex == GraphDataSource::invalidIndex())
        return TableSelectionOptional(); // Either column was defined but not found.

    // Special handling for number generator: unless specified otherwise, generated values are created as (values, rows) instead of default (rows, values)
    if (dynamic_cast<const NumberGeneratorDataSource*>(&source) != nullptr && xColumnIndex == nDefaultValue && yColumnIndex == nDefaultValue)
        yColumnIndex = nRowColumnSpecifier;

    auto& bXisRowIndex = rRowFlags[0];
    auto& bYisRowIndex = rRowFlags[1];
    auto& bZisRowIndex = rRowFlags[2];
    bXisRowIndex = isRowIndexSpecifier(xColumnIndex);
    bYisRowIndex = isRowIndexSpecifier(yColumnIndex);
    bZisRowIndex = isRowIndexSpecifier(zColumnIndex);

    if (bXisRowIndex && bYisRowIndex)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
            defEntry.log(GraphDefinitionEntry::LogLevel::error, defEntry.tr("Both columns are specified as row_index"));
        return TableSelectionOptional();
    }

    if (source.columnCount() == 1 && !bXisRowIndex && !bYisRowIndex)
    {
        if (xColumnIndex == nDefaultValue)
            bXisRowIndex = true;
        else if (yColumnIndex == nDefaultValue)
            bYisRowIndex = true;
    }

    makeEffectiveColumnIndexes(xColumnIndex, yColumnIndex, zColumnIndex, columns, isRowIndexSpecifier);

    const auto bStringsNeededForX = (!bXisRowIndex && (defEntry.isType(ChartObjectChartTypeStr_bars) || (defEntry.isType(ChartObjectChartTypeStr_histogram) && defEntry.fieldValueStr(ChartObjectFieldIdStr_binType) == DFG_UTF8("text"))));
    const auto bStringsNeededForY = (!bYisRowIndex && defEntry.isType(ChartObjectChartTypeStr_histogram) && defEntry.fieldValueStr(ChartObjectFieldIdStr_binType) == DFG_UTF8("text"));
    const auto bStringsNeededForZ = (!bZisRowIndex && defEntry.isType(ChartObjectChartTypeStr_txys));

    const auto bXsuccess = (bStringsNeededForX) ? rCacheItem.storeColumnFromSource_strings(source, xColumnIndex) : rCacheItem.storeColumnFromSource(source, xColumnIndex);
    bool bYsuccess = false;
    bool bZsuccess = false;
    if (bXsuccess)
        bYsuccess = (bStringsNeededForY) ? rCacheItem.storeColumnFromSource_strings(source, yColumnIndex) : rCacheItem.storeColumnFromSource(source, yColumnIndex);
    if (bYsuccess)
        bZsuccess = (!defEntry.isType(ChartObjectChartTypeStr_txys) || bZisRowIndex || rCacheItem.storeColumnFromSource_strings(source, zColumnIndex));

    if (bXsuccess && bYsuccess && bZsuccess)
        return iter->second;
    else
        return TableSelectionOptional(); // Failed to read columns
}

void DFG_MODULE_NS(qt)::ChartDataCache::removeInvalidCaches()
{
    ::DFG_MODULE_NS(cont)::eraseIf(m_tableSelectionDatas, [](decltype(*m_tableSelectionDatas.begin())& pairItem)
    {
        auto& opt = pairItem.second;
        return !opt || opt->isVolatileCache() || !opt->isValid();
    });
}

auto ChartDataCache::cacheKey(const GraphDataSource& source, const AbstractChartControlItem& defEntry) const -> CacheEntryKey
{
    DFG_UNUSED(defEntry);
    return source.uniqueId();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   ChartDefinition
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ChartDefinition::ChartDefinition(const QString& sJson, GraphDataSourceId defaultSourceId)
    : m_defaultSourceId(std::move(defaultSourceId))
{
    m_controlEntries = sJson.split('\n');
}

template <class While_T, class Func_T>
void ChartDefinition::forEachEntryWhile(While_T&& whileFunc, Func_T&& handler) const
{
    int i = 0;
    for (const auto& sEntry : m_controlEntries)
    {
        if (!whileFunc())
            return;
        if (!sEntry.isEmpty() && sEntry[0] != '#')
            handler(GraphDefinitionEntry::fromText(sEntry, i++));
    }
}

template <class Func_T>
void ChartDefinition::forEachEntry(Func_T&& handler) const
{
    forEachEntryWhile([] { return true; }, std::forward<Func_T>(handler));
}

size_t ChartDefinition::getForEachEntryCount() const
{
    size_t nCount = 0;
    forEachEntry([&](Dummy) { ++nCount; });
    return nCount;
}

bool ChartDefinition::isSourceUsed(const GraphDataSourceId& sourceId, bool* pHasErrorEntries) const
{
    bool bFound = false;
    if (pHasErrorEntries)
        *pHasErrorEntries = false;
    forEachEntryWhile([&] { return !bFound; }, [&](const GraphDefinitionEntry& entry)
    {
        if (entry.isEnabled() && sourceId == entry.sourceId(this->m_defaultSourceId))
            bFound = true;
        if (pHasErrorEntries && *pHasErrorEntries == false && !entry.fieldValueStr(ChartObjectFieldIdStr_errorString).empty())
            *pHasErrorEntries = true;
    });
    return bFound;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDefinitionWidget
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Defines UI for controlling chart entries
// Requirements:
//      -Must provide text-based export/import of settings in order to easily store/restore previous settings.
class GraphDefinitionWidget : public QWidget
{
public:
    typedef QWidget BaseClass;

    GraphDefinitionWidget(GraphControlPanel *pNonNullParent);

    QString getRawTextDefinition() const;

    void addJsonWidgetContextMenuEntries(JsonListWidget& rJsonListWidget);

    void showGuideWidget();

    static QString getGuideString();
    void setGuideString(const QString& s);

    ChartDefinition getChartDefinition();

    ChartDefinitionViewer getChartDefinitionViewer();

    ChartController* getController();

    void onTextDefinitionChanged();

    void updateChartDefinitionViewable();
    void checkUpdateChartDefinitionViewableTimer();

    void onActionButtonClicked();

    QObjectStorage<JsonListWidget> m_spRawTextDefinition; // Guaranteed to be non-null between constructor and destructor.
    QObjectStorage<QWidget> m_spGuideWidget;
    QObjectStorage<QPushButton> m_spApplyButton;
    QPointer<GraphControlPanel> m_spParent;
    ChartDefinitionViewable m_chartDefinitionViewable;
    QElapsedTimer m_timeSinceLastEdit;
    QTimer m_chartDefinitionTimer;
    const int m_nChartDefinitionUpdateMinimumIntervalInMs = 250;

    static QString s_sGuideString;
    static const char s_szApplyText[];
    static const char s_szTerminateText[];
}; // Class GraphDefinitionWidget

QString GraphDefinitionWidget::s_sGuideString;
const char GraphDefinitionWidget::s_szApplyText[]     = QT_TR_NOOP("Apply");
const char GraphDefinitionWidget::s_szTerminateText[] = QT_TR_NOOP("Terminate");

GraphDefinitionWidget::GraphDefinitionWidget(GraphControlPanel *pNonNullParent) :
    BaseClass(pNonNullParent),
    m_spParent(pNonNullParent)
{
    DFG_ASSERT(m_spParent.data() != nullptr);

    auto spLayout = std::make_unique<QVBoxLayout>(this);

    m_spRawTextDefinition.reset(new JsonListWidget(this));
    m_spRawTextDefinition->setPlainText(GraphDefinitionEntry::xyGraph(QString(), QString()).toText());
    addJsonWidgetContextMenuEntries(*m_spRawTextDefinition);
    DFG_QT_VERIFY_CONNECT(connect(m_spRawTextDefinition.get(), &JsonListWidget::textChanged, this, &GraphDefinitionWidget::onTextDefinitionChanged));

    spLayout->addWidget(m_spRawTextDefinition.get());

    // Adding control buttons
    {
        auto spHlayout = std::make_unique<QHBoxLayout>();
        m_spApplyButton.reset(new QPushButton(tr(s_szApplyText), this));
        auto pGuideButton = new QPushButton(tr("Show guide"), this); // Deletion through parentship.
        pGuideButton->setObjectName("guideButton");
        pGuideButton->setHidden(true);

        DFG_QT_VERIFY_CONNECT(connect(m_spApplyButton.get(), &QPushButton::clicked, this, &GraphDefinitionWidget::onActionButtonClicked));
        DFG_QT_VERIFY_CONNECT(connect(pGuideButton, &QPushButton::clicked, this, &GraphDefinitionWidget::showGuideWidget));

        spHlayout->addWidget(m_spApplyButton.get());
        spHlayout->addWidget(pGuideButton);
        spLayout->addLayout(spHlayout.release()); // spHlayout is made child of spLayout.
    }

    setLayout(spLayout.release()); // Ownership transferred to *this.

    // Setting m_chartDefinitionTimer
    {
        DFG_QT_VERIFY_CONNECT(connect(&m_chartDefinitionTimer, &QTimer::timeout, this, &GraphDefinitionWidget::checkUpdateChartDefinitionViewableTimer));
        m_chartDefinitionTimer.setSingleShot(true);
    }
}

void GraphDefinitionWidget::addJsonWidgetContextMenuEntries(JsonListWidget& rJsonListWidget)
{
    // 'Insert'-items
    auto pJsonListWidget = &rJsonListWidget;
    {
        const auto addInsertAction = [&](const QString& sTitle, const QString& sInsertText)
        {
            std::unique_ptr<QAction> spAction(new QAction(sTitle, &rJsonListWidget));
            
            const auto handler = [=]()
            {
                pJsonListWidget->insertPlainText(sInsertText);
            };
            DFG_QT_VERIFY_CONNECT(connect(spAction.get(), &QAction::triggered, handler));
            rJsonListWidget.addAction(spAction.release()); // Deletion through parentship.
        };
        addInsertAction(tr("Insert basic 'xy'"),        R"({ "type":"xy" })");
        addInsertAction(tr("Insert basic 'txy'"),       R"({ "type":"txy" })");
        addInsertAction(tr("Insert basic 'txys'"),      R"({ "type":"txys" })");
        addInsertAction(tr("Insert basic 'bars'"),      R"({ "type":"bars" })");
        addInsertAction(tr("Insert basic 'histogram'"), R"({ "type":"histogram" })");
        addInsertAction(tr("Insert basic 'panel_config'"),
            R"({"type": "panel_config", "title": "Panel title", "x_label": "x label", "y_label": "y label"})");
        addInsertAction(tr("Insert basic 'global_config'"),
            R"({ "type":"global_config", "show_legend":true })");
    }
}

void GraphDefinitionWidget::onTextDefinitionChanged()
{
    m_timeSinceLastEdit.start();
    if (!m_chartDefinitionTimer.isActive())
        m_chartDefinitionTimer.start(m_nChartDefinitionUpdateMinimumIntervalInMs);
}

void GraphDefinitionWidget::updateChartDefinitionViewable()
{
    auto newChartDef = getChartDefinition();
    m_chartDefinitionViewable.edit([&](ChartDefinition& cd, const ChartDefinition* pOld)
    {
        DFG_UNUSED(pOld);
        cd = std::move(newChartDef);
    });
}

void GraphDefinitionWidget::checkUpdateChartDefinitionViewableTimer()
{
    if (!m_timeSinceLastEdit.isValid())
        return;
    const auto elapsed = m_timeSinceLastEdit.elapsed();
    const auto nEffectiveLimit = m_nChartDefinitionUpdateMinimumIntervalInMs * 19 / 20; // Allowing some inaccuracies (5 %) so that if this function gets called e.g. at 249 ms with limit 250 ms, interpreting as close enough.
    if (elapsed >= nEffectiveLimit)
        updateChartDefinitionViewable();
    else // case: not time to update yet, scheduling new check.
        m_chartDefinitionTimer.start(m_nChartDefinitionUpdateMinimumIntervalInMs - static_cast<int>(elapsed));
}

void GraphDefinitionWidget::setGuideString(const QString& s)
{
    if (s_sGuideString.isEmpty())
    {
        s_sGuideString = s;
        auto pGuideButton = this->findChild<QPushButton*>("guideButton");
        if (pGuideButton)
            pGuideButton->setHidden(false);
    }
    else
    {
        DFG_ASSERT_CORRECTNESS(false); // Guide string can be set only once.
    }
}

QString GraphDefinitionWidget::getGuideString()
{
    // Originally guide string was embedded - while waiting for std::embed (http://open-std.org/JTC1/SC22/WG21/docs/papers/2020/p1040r6.html) -  here like this:
    //return QString(
    //#include "res/chartGuide.html"
    //);
    // The file, however, was starting to get too big (~16 kB for MSVC2017) for this pattern causing compile error.
    // And while this is in Qt code, using qrc resources seemed tricky as being part of library, 
    // there's no executable or compiled library to embed to without introducing additional build requirements.
    // So the solution for now is to require the guide to be set from outside, where it hopefully is easier to embed e.g. as qrc-resource.
    return s_sGuideString;
}

void GraphDefinitionWidget::showGuideWidget()
{
    if (!m_spGuideWidget)
    {
        m_spGuideWidget.reset(new QDialog(this));
        m_spGuideWidget->setWindowTitle(tr("Chart guide"));
        auto pTextEdit = new QTextEdit(m_spGuideWidget.get()); // Deletion through parentship.
        auto pLayout = new QVBoxLayout(m_spGuideWidget.get());
        pTextEdit->setTextInteractionFlags(Qt::TextBrowserInteraction);
        pTextEdit->setHtml(getGuideString());

        // At least on Qt 5.13 with MSVC2017 default font size was a bit small (8.25), increasing to 11.
        {
            auto font = pTextEdit->currentFont();
            const auto oldPointSizeF = font.pointSizeF();
            font.setPointSizeF(Max(qreal(11), oldPointSizeF));
            pTextEdit->setFont(font);
        }

        // Creating edit for find text and adding it to layout
        auto pFindEdit = new QLineEdit(m_spGuideWidget.get()); // Deletion through parentship.
        pFindEdit->setMaxLength(1000);
        pFindEdit->setPlaceholderText("Find... (Ctrl+F)");
        pLayout->addWidget(pFindEdit);

        // Adjusting colors in text edit so that highlighted text shows more clearly even if the text edit does not have focus.
        {
            QPalette pal = pTextEdit->palette();
            pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
            pTextEdit->setPalette(pal);
        }

        // Adding shortcut Ctrl+F (activate find edit)
        {
            auto pActivateFind = new QAction(pTextEdit);
            pActivateFind->setShortcut(QString("Ctrl+F"));
            m_spGuideWidget->addAction(pActivateFind); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActivateFind, &QAction::triggered, pFindEdit, [=]() { pFindEdit->setFocus(); pFindEdit->selectAll(); }));
        }

        const auto wrapCursor = [](QTextEdit* pTextEdit, QLineEdit* pFindEdit, const QTextDocument::FindFlags findFlags, const QTextCursor::MoveOperation moveOp)
            {
                if (!pTextEdit || !pFindEdit)
                    return;
                if (!pTextEdit->find(pFindEdit->text(), findFlags)) // If didn't find, wrapping to begin/end and trying to find again.
                {
                    const auto initialCursor = pTextEdit->textCursor();
                    pTextEdit->moveCursor(moveOp);
                    if (!pTextEdit->find(pFindEdit->text(), findFlags))
                        pTextEdit->setTextCursor(initialCursor); // Didn't find after wrapping so restoring cursor position.
                }
            };

        // Adding shortcut F3 (find)
        {
            auto pActionF3 = new QAction(m_spGuideWidget.get());
            pActionF3->setShortcut(QString("F3"));
            m_spGuideWidget->addAction(pActionF3); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActionF3, &QAction::triggered, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindFlags(), QTextCursor::Start); }));
        }
        
        // Adding shortcut Shift+F3 (find backward)
        {
            auto pActionShiftF3 = new QAction(m_spGuideWidget.get());
            pActionShiftF3->setShortcut(QString("Shift+F3"));
            m_spGuideWidget->addAction(pActionShiftF3); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActionShiftF3, &QAction::triggered, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindBackward, QTextCursor::End); }));
        }

        // Connecting textChanged() to trigger find.
        DFG_QT_VERIFY_CONNECT(connect(pFindEdit, &QLineEdit::textChanged, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindFlags(), QTextCursor::Start); }));
        
        pLayout->addWidget(pTextEdit);
        removeContextHelpButtonFromDialog(m_spGuideWidget.get());

        // Resizing guide window to 75 % of mainwindow size (if mainwindow is available) or at least (600, 500)
        {
            auto pMainWindow = getWidgetByType<QMainWindow>(QApplication::topLevelWidgets());
            const auto width = Max(600, (pMainWindow) ? 3 * pMainWindow->width() / 4 : 0);
            const auto height = Max(500, (pMainWindow) ? 3 * pMainWindow->height() / 4 : 0);
            m_spGuideWidget->resize(width, height);
        }
    }
    m_spGuideWidget->show();
}

auto GraphDefinitionWidget::getChartDefinition() -> ChartDefinition
{
    auto pController = getController();
    auto defaultSource = (pController) ? pController->defaultSourceId() : GraphDataSourceId();
    return ChartDefinition((m_spRawTextDefinition) ? m_spRawTextDefinition->toPlainText() : QString(), defaultSource);
}

auto GraphDefinitionWidget::getChartDefinitionViewer() -> ChartDefinitionViewer
{
    this->updateChartDefinitionViewable();
    return m_chartDefinitionViewable.createViewer();
}

QString GraphDefinitionWidget::getRawTextDefinition() const
{
    return (m_spRawTextDefinition) ? m_spRawTextDefinition->toPlainText() : QString();
}

ChartController* GraphDefinitionWidget::getController()
{
    return (m_spParent) ? m_spParent->getController() : nullptr;
}

void GraphDefinitionWidget::onActionButtonClicked()
{
    auto pButton = m_spApplyButton.get();
    if (!pButton)
        return;

    auto pController = getController();
    if (!pController)
        return;

    const auto sButtonText = pButton->text();
    if (sButtonText == tr(s_szApplyText))
        pController->refresh();
    else if (sButtonText.startsWith(tr(s_szTerminateText)))
    {
        pController->refreshTerminate();
        pButton->setText("Terminating...");
        pButton->setEnabled(false);
    }
}

}} // module namespace


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphControlPanel
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DFG_MODULE_NS(qt)::GraphControlPanel::GraphControlPanel(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
    m_spGraphDefinitionWidget.reset(new GraphDefinitionWidget(this));

    // Creating console display.
    {
        auto pConsole = new ConsoleDisplay(this);
        DFG_DETAIL_NS::gConsoleLogHandle.setHandler([=](const QString& s, DFG_DETAIL_NS::ConsoleLogLevel logLevel) { pConsole->addEntry(s, DFG_DETAIL_NS::consoleLogLevelToEntryType(logLevel)); } );

        // Adding action for controlling cache detail logging.
        {
            auto pAction = new QAction(tr("Enable cache detail logging"), this); // Deletion through parentship.
            pAction->setCheckable(true);
            m_bLogCacheDiagnosticsOnUpdate = (DFG_BUILD_DEBUG_RELEASE_TYPE == StringViewC("debug"));
            pAction->setChecked(m_bLogCacheDiagnosticsOnUpdate);
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::toggled, this, [&](const bool b) { m_bLogCacheDiagnosticsOnUpdate = b; }));
            pConsole->addAction(pAction);
        }
        m_spConsoleWidget.reset(pConsole);
    }

    // Adding some controls
    {
        std::unique_ptr<QHBoxLayout> pFirstRowLayout(new QHBoxLayout);

        // enable / disabled control
        {
            auto pEnableCheckBox = new QCheckBox(tr("Enable"), this); // Parent owned.
            pEnableCheckBox->setObjectName("cb_enabled");
            DFG_QT_VERIFY_CONNECT(connect(pEnableCheckBox, &QCheckBox::toggled, this, &GraphControlPanel::sigGraphEnableCheckboxToggled));
            pFirstRowLayout->addWidget(pEnableCheckBox);
            pEnableCheckBox->setChecked(true);
        }
        {
            {
                auto pShowControlsCheckBox = new QCheckBox(tr("Show controls"), this); // Parent owned
                DFG_QT_VERIFY_CONNECT(connect(pShowControlsCheckBox, &QCheckBox::toggled, this, &GraphControlPanel::onShowControlsCheckboxToggled));
                pShowControlsCheckBox->setChecked(true);
                pFirstRowLayout->addWidget(pShowControlsCheckBox);
            }

            {
                auto pShowConsole = new QCheckBox(tr("Show console"), this); // Parent owned
                DFG_QT_VERIFY_CONNECT(connect(pShowConsole, &QCheckBox::toggled, this, &GraphControlPanel::onShowConsoleCheckboxToggled));
                pShowConsole->setChecked(true);
                pFirstRowLayout->addWidget(pShowConsole);
            }
        }

        pLayout->addLayout(pFirstRowLayout.release(), 0, 0); // pFirstRowLayout becomes child of pLayout so releasing for parent deletion.
    }

    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_spGraphDefinitionWidget.get());
    pLayout->addWidget(m_spConsoleWidget.get());
}

DFG_MODULE_NS(qt)::GraphControlPanel::~GraphControlPanel()
{
    DFG_DETAIL_NS::gConsoleLogHandle.setHandler(nullptr);
}

void DFG_MODULE_NS(qt)::GraphControlPanel::onShowControlsCheckboxToggled(bool b)
{
    if (m_spGraphDefinitionWidget)
    {
        m_spGraphDefinitionWidget->setVisible(b);
        Q_EMIT sigPreferredSizeChanged(sizeHint());
    }
}

void DFG_MODULE_NS(qt)::GraphControlPanel::onShowConsoleCheckboxToggled(const bool b)
{
    if (m_spConsoleWidget)
    {
        m_spConsoleWidget->setVisible(b);
        Q_EMIT sigPreferredSizeChanged(sizeHint());
    }
}

void ::DFG_MODULE_NS(qt)::GraphControlPanel::onDisplayStateChanged(const ChartDisplayState state)
{
    auto pDefWidget = dynamic_cast<GraphDefinitionWidget*>(m_spGraphDefinitionWidget.get());
    auto pButton = (pDefWidget) ? pDefWidget->m_spApplyButton.get() : nullptr;
    if (!pButton)
        return;
    QString sText;
    QString sToolTip;
    QString sStyleSheet;
    bool bEnable = true;
    switch (state)
    {
        case ChartDisplayState::idle:       sText = tr(GraphDefinitionWidget::s_szApplyText); break;
        case ChartDisplayState::updating:
            {
                sText = tr(GraphDefinitionWidget::s_szTerminateText);
                if (state.totalUpdateStepCount() > 0)
                {
                    sText += QString(" (preparing %1/%2)").arg(state.completedUpdateStepCount()).arg(state.totalUpdateStepCount());
                    const double progress = double(state.completedUpdateStepCount()) / double(state.totalUpdateStepCount());
                    // Stylesheet solution adapted from https://forum.qt.io/topic/82456/how-to-design-a-qpushbutton-with-progress/4
                    // Using palette of QPushButton directly didn't seem to work (setting gradient brush to QPalette::Button role didn't affect background
                    // and some of the hints in https://stackoverflow.com/questions/21685414/qt5-setting-background-color-to-qpushbutton-and-qcheckbox didn't help either)
                    sStyleSheet = QString(
                        "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,"
                        "stop:0 rgba(0, 200, 0, 255), stop:%1 rgba(0, 255, 0, 255),"
                        "stop:%2 rgba(255, 255, 255, 255), stop:1 rgba(255, 255, 255, 255));")
                        .arg(progress - 0.001f)
                        .arg(progress);
                }
                sToolTip = tr("If update is terminated, currently processed chart entries are finished before terminating");
            }
            break;
        case ChartDisplayState::finalizing: sText = tr("Finalizing..."); bEnable = false; break;
        default: sText = tr("Bug"); break;
    }
    pButton->setStyleSheet(sStyleSheet);
    pButton->setText(sText);
    pButton->setToolTip(sToolTip);
    pButton->setEnabled(bEnable);
    pButton->repaint(); // Note: must use repaint() instead of update() as some state changes may effectively freeze GUI so can't wait for update() to happen.
}

auto DFG_MODULE_NS(qt)::GraphControlPanel::getController() -> ChartController*
{
    return getControllerFromParents(this);
}

bool DFG_MODULE_NS(qt)::GraphControlPanel::getEnabledFlag() const
{
    auto pCbEnabled = findChild<const QCheckBox*>(QLatin1String("cb_enabled"));
    return (pCbEnabled) ? pCbEnabled->isChecked() : true;
}

void DFG_MODULE_NS(qt)::GraphControlPanel::setEnabledFlag(const bool b)
{
    auto pCbEnabled = findChild<QCheckBox*>(QLatin1String("cb_enabled"));
    if (pCbEnabled)
        pCbEnabled->setChecked(b);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDisplay
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DFG_MODULE_NS(qt)::GraphDisplay::GraphDisplay(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
    auto pChartCanvas = new ChartCanvasQCustomPlot(this);
    auto pWidget = pChartCanvas->getWidget();
    if (pWidget)
    {
        pLayout->addWidget(pWidget);
        // Setting context menu handling. Note that pWidget->setContextMenuPolicy(Qt::NoContextMenu) doesn't work because QCustomPlot overrides mousePressEvent().
        // Using pWidget->setAttribute(Qt::WA_TransparentForMouseEvents) would make context menu work, but also disables e.g. QCustomPlot's zoom controls.
        DFG_QT_VERIFY_CONNECT(connect(pWidget, &QCustomPlot::mousePress, [=](QMouseEvent* pEvent) { if (pEvent && pEvent->button() == Qt::RightButton) contextMenuEvent(nullptr); }));
    }
    m_spChartCanvas.reset(pChartCanvas);
#else // Case: no graph display implementation available, creating a placeholder widget.
    auto pTextEdit = new QPlainTextEdit(this);
    pTextEdit->appendPlainText(tr("Placeholder"));
    pLayout->addWidget(pTextEdit);
#endif
}

DFG_MODULE_NS(qt)::GraphDisplay::~GraphDisplay()
{

}

auto DFG_MODULE_NS(qt)::GraphDisplay::chart() -> ChartCanvas*
{
    return m_spChartCanvas.get();
}

void DFG_MODULE_NS(qt)::GraphDisplay::showSaveAsImageDialog()
{
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
    auto pChart = chart();
    auto pCustomPlotChart = dynamic_cast<ChartCanvasQCustomPlot*>(pChart);
    if (!pCustomPlotChart)
        return;
    GraphDisplayImageExportDialog dlg(pCustomPlotChart->getWidget());
    dlg.exec();
#else
    QMessageBox::information(this, tr("Not implemented"), tr("Not implemented"));
#endif
}

void DFG_MODULE_NS(qt)::GraphDisplay::contextMenuEvent(QContextMenuEvent* pEvent)
{
    DFG_UNUSED(pEvent);
    if (!m_spChartCanvas)
        return;

    // Hack: accessing GraphControlAndDisplayWidget in a questionable manner.
    GraphControlAndDisplayWidget* pParentGraphWidget = [=]() { auto pParent = parent(); return (pParent) ? qobject_cast<GraphControlAndDisplayWidget*>(pParent->parent()) : nullptr; }();
    if (!pParentGraphWidget)
    {
        DFG_ASSERT(false);
        return;
    }

    QMenu menu;

    // Global options
    {
        menu.addAction(tr("Refresh"), pParentGraphWidget, &GraphControlAndDisplayWidget::refresh);
        menu.addAction(tr("Rescale all axis"), this, [=]() { auto pChart = this->chart(); if (pChart) pChart->optimizeAllAxesRanges(); });

        auto pRemoveAllAction = menu.addAction(tr("Remove all chart objects"), pParentGraphWidget, [&]() { this->m_spChartCanvas->removeAllChartObjects(); });
        if (pRemoveAllAction && !this->m_spChartCanvas->hasChartObjects())
            pRemoveAllAction->setDisabled(true);

        if (m_spChartCanvas->isLegendSupported())
        {
            auto pShowLegendAction = menu.addAction(tr("Show legend"), pParentGraphWidget, [&](bool b) { this->m_spChartCanvas->enableLegend(b); });
            pShowLegendAction->setCheckable(true);
            pShowLegendAction->setChecked(m_spChartCanvas->isLegendEnabled());
        }

        if (m_spChartCanvas->isToolTipSupported())
        {
            auto pToggleToolTipAction = menu.addAction(tr("Show tooltip"), pParentGraphWidget, [&](bool b) { this->m_spChartCanvas->enableToolTip(b); });
            pToggleToolTipAction->setCheckable(true);
            pToggleToolTipAction->setChecked(m_spChartCanvas->isToolTipEnabled());
        }

#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
        // Export actions
        {
            menu.addAction(tr("Save as image..."), this, &GraphDisplay::showSaveAsImageDialog);
        }
#endif // QCustomPlot

        // Clear caches -action
        menu.addAction(tr("Clear chart caches"), this, &GraphDisplay::clearCaches);
    }

    addSectionEntryToMenu(&menu, tr("Chart objects"));
    m_spChartCanvas->addContextMenuEntriesForChartObjects(&menu);

    menu.exec(QCursor::pos());
}

void DFG_MODULE_NS(qt)::GraphDisplay::clearCaches()
{
    auto pController = getController();
    if (pController)
        pController->clearCaches();
    else
        DFG_QT_CHART_CONSOLE_WARNING("Failed to clear caches: no controller object");
}

auto DFG_MODULE_NS(qt)::GraphDisplay::getController() -> ChartController*
{
    return getControllerFromParents(this);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphControlAndDisplayWidget
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::RefreshContext
{
public:
};

class DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartData : public ::DFG_MODULE_NS(charts)::ChartOperationPipeData
{
public:
    using BaseClass = ::DFG_MODULE_NS(charts)::ChartOperationPipeData;

    ChartDataType columnDataType(const size_t c) const
    {
        return isValidIndex(m_columnDataTypes, c) ? m_columnDataTypes[c] : ChartDataType();
    }
    QString columnName(const size_t c) const
    {
        return isValidIndex(m_columnNames, c) ? m_columnNames[c] : QString();
    }

    ChartData& operator=(::DFG_MODULE_NS(charts)::ChartOperationPipeData&& other)
    {
        BaseClass::operator=(std::move(other));
        return *this;
    }

    // Note: this differs from move assignment: move assignment moves internal data buffers from other to this, while
    //       this function moves externally owned data to internal buffers.
    void copyOrMoveDataFrom(::DFG_MODULE_NS(charts)::ChartOperationPipeData& other)
    {
        this->m_vectorRefs.resize(other.vectorCount());
        std::vector<bool> stringFlags(this->m_vectorRefs.size(), false);
        for (size_t i = 0, nCount = other.vectorCount(); i < nCount; ++i)
        {
            auto pValues = other.valuesByIndex(i);
            if (pValues)
                *this->editableValuesByIndex(i) = std::move(*pValues);
            else
            {
                auto pStrings = other.stringsByIndex(i);
                if (pStrings)
                {
                    *this->editableStringsByIndex(i) = std::move(*pStrings);
                    stringFlags[i] = true;
                }
            }
        }
        // Updating references.
        for (size_t i = 0, nCount  = stringFlags.size(); i < nCount; ++i)
        {
            if (stringFlags[i])
                this->m_vectorRefs[i] = this->editableStringsByIndex(i);
            else
                this->m_vectorRefs[i] = this->editableValuesByIndex(i);
        }
    }

    std::vector<ChartDataType> m_columnDataTypes;
    std::vector<QString> m_columnNames;
    bool m_bXisRowIndex = false;
    bool m_bYisRowIndex = false;
}; // class ChartData

// ChartRefreshParam
DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam)
{
public:
    ChartDefinition m_chartDefinition;
    DataSourceContainer m_sources;
    ::DFG_MODULE_NS(cont)::MapVectorSoA<int, ChartData> m_preparedData; // Index refers to index in m_chartDefinition.
    std::shared_ptr<ChartDataCache> m_spCache;
    bool m_bTerminated = false;
};

// Opaque member definition for GraphControlAndDisplayWidget
DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartDataPreparator)
{
    std::atomic_bool m_terminateFlag;
};

::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartDataPreparator::~ChartDataPreparator() = default;

void ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartDataPreparator::prepareData(ChartRefreshParamPtr spParam)
{
    DFG_OPAQUE_REF().m_terminateFlag = false;
    if (spParam)
    {
        const auto& chartDefinition = spParam->chartDefinition();
        const auto pCurrentThread = QThread::currentThread();
        const auto totalForEachCount = saturateCast<int>(chartDefinition.getForEachEntryCount());
        int progressCounter = 0;
        chartDefinition.forEachEntry([&](const GraphDefinitionEntry& entry)
        {
            if (DFG_OPAQUE_REF().m_terminateFlag)
                return;
            if (entry.isEnabled())
            {
                auto& sources = spParam->dataSources();
                const auto sSourceId = entry.sourceId(chartDefinition.m_defaultSourceId);
                auto iterSource = sources.findById(sSourceId);
                if (iterSource == sources.end())
                    iterSource = tryCreateOnDemandDataSource(sSourceId, sources);
                if (iterSource != sources.end() && sources.iterToRef(iterSource).isSafeToQueryDataFromThread(pCurrentThread))
                    spParam->storePreparedData(entry, GraphControlAndDisplayWidget::prepareData(spParam->cache(), sources.iterToRef(iterSource), entry));
            }
            ++progressCounter;
            Q_EMIT sigOnEntryPrepared(EntryPreparedParam(progressCounter, totalForEachCount));
        });
        if (DFG_OPAQUE_REF().m_terminateFlag)
            spParam->setTerminatedFlag(true);
    }
    Q_EMIT sigPreparationFinished(std::move(spParam));
}

void ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartDataPreparator::terminatePreparation()
{
    DFG_OPAQUE_REF().m_terminateFlag = true;
}


////////////////////////////////////////////////////
//
//
// GraphControlAndDisplayWidget::EntryPreparedParam
// 
//
////////////////////////////////////////////////////

::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::EntryPreparedParam::EntryPreparedParam(const int nTotalPrepareReadyCount, const int nTotalPrepareCount)
    : m_nTotalPrepareReadyCount(nTotalPrepareReadyCount)
    , m_nTotalPrepareCount(nTotalPrepareCount)
{

}

int ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::EntryPreparedParam::totalPrepareReadyCount() const
{
    return this->m_nTotalPrepareReadyCount;
}

int ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::EntryPreparedParam::totalPrepareCount() const
{
    return this->m_nTotalPrepareCount;
}

// Opaque member definition for GraphControlAndDisplayWidget
DFG_OPAQUE_PTR_DEFINE(DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget)
{
    ::DFG_MODULE_NS(time)::TimerCpu m_refreshTimer; // Used for measuring refresh times.
    QPointer<QThread> m_spThreadDataPreparation;
    QPointer<ChartDataPreparator> m_spChartPreparator; // Lives in thread m_spThreadDataPreparation, created on first use, deleted in destructor.
    size_t m_nRefreshCounter = 0; // Keeps count of refreshes.
};

::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::ChartRefreshParam() = default;
::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::ChartRefreshParam(const ChartRefreshParam& other) = default;
::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::~ChartRefreshParam() = default;

::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::ChartRefreshParam(const ChartDefinition& chartDefinition, DataSourceContainer sources, std::shared_ptr<ChartDataCache> spCache)
{
    DFG_OPAQUE_REF().m_chartDefinition = chartDefinition;
    DFG_OPAQUE_REF().m_sources = std::move(sources);
    DFG_OPAQUE_REF().m_spCache = std::move(spCache);
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::chartDefinition() const -> const ChartDefinition&
{
    DFG_ASSERT_UB(DFG_OPAQUE_PTR() != nullptr);
    return DFG_OPAQUE_PTR()->m_chartDefinition;
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::dataSources() -> DataSourceContainer&
{
    return DFG_OPAQUE_REF().m_sources;
}

void ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::storePreparedData(const GraphDefinitionEntry& defEntry, ChartData chartData)
{
    DFG_OPAQUE_REF().m_preparedData.insert(defEntry.index(), std::move(chartData));
    DFG_ASSERT(chartData.m_valueVectors.empty() && chartData.m_stringVectors.empty()); // Verifying that didn't make redundant copies.
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::preparedDataForEntry(const GraphDefinitionEntry& defEntry) -> ChartData*
{
    auto iter = DFG_OPAQUE_REF().m_preparedData.find(defEntry.index());
    return  (iter != DFG_OPAQUE_REF().m_preparedData.end()) ? &iter->second : nullptr;
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::cache() -> std::shared_ptr<ChartDataCache>&
{
    return DFG_OPAQUE_REF().m_spCache;
}

void ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::setTerminatedFlag(const bool b)
{
    DFG_OPAQUE_REF().m_bTerminated = b;
}

bool ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::ChartRefreshParam::wasTerminated() const
{
    return DFG_OPAQUE_PTR() && DFG_OPAQUE_PTR()->m_bTerminated;
}

DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::GraphControlAndDisplayWidget(const bool bAllowAppSettingUsage)
{
    this->setProperty(gPropertyIdAllowAppSettingsUsage, bAllowAppSettingUsage);
    auto pLayout = new QGridLayout(this);
    m_spSplitter.reset(new QSplitter(Qt::Vertical, this));
    m_spControlPanel.reset(new GraphControlPanel(this));

    m_spGraphDisplay.reset(new GraphDisplay(this));
    m_spSplitter->addWidget(m_spControlPanel.data());
    m_spSplitter->addWidget(m_spGraphDisplay.data());
    pLayout->addWidget(m_spSplitter.get());

    DFG_QT_VERIFY_CONNECT(connect(m_spControlPanel.get(), &GraphControlPanel::sigPreferredSizeChanged, this, &GraphControlAndDisplayWidget::onControllerPreferredSizeChanged));
    DFG_QT_VERIFY_CONNECT(connect(m_spControlPanel.get(), &GraphControlPanel::sigGraphEnableCheckboxToggled, this, &GraphControlAndDisplayWidget::onGraphEnableCheckboxToggled));

    onControllerPreferredSizeChanged(m_spControlPanel->sizeHint());

    const auto nCurrentHeight = this->height();
    const auto nGraphWidgetHeight = nCurrentHeight * 3 / 4;
    m_spSplitter->setSizes(QList<int>() << nCurrentHeight - nGraphWidgetHeight << nGraphWidgetHeight);

    this->setFrameShape(QFrame::Panel);
}

DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::~GraphControlAndDisplayWidget()
{
    // From Qt documentation "Threads and QObjects": "You must ensure that all objects created in a thread are deleted before you delete the QThread.This can be done easily by creating the objects on the stack in your run() implementation."
    this->m_dataSources.m_sources.clear();
    if (DFG_OPAQUE_REF().m_spChartPreparator)
    {
        DFG_OPAQUE_REF().m_spChartPreparator->deleteLater();
        DFG_OPAQUE_REF().m_spChartPreparator = nullptr;
    }
    // Stopping preparation thread.
    if (DFG_OPAQUE_REF().m_spThreadDataPreparation)
    {
        DFG_OPAQUE_REF().m_spThreadDataPreparation->quit();
        DFG_OPAQUE_REF().m_spThreadDataPreparation->deleteLater();
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setChartGuide(const QString& s)
{
    auto p = getDefinitionWidget();
    if (p)
        p->setGuideString(s);
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::getChartDefinitionString() const -> QString
{
    auto p = getDefinitionWidget();
    return (p) ? p->getRawTextDefinition() : QString();
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onControllerPreferredSizeChanged(const QSize sizeHint)
{
    if (m_spSplitter)
    {
        const auto nHeight = this->size().height();
        auto sizes = m_spSplitter->sizes();
        DFG_ASSERT_CORRECTNESS(sizes.size() >= 2);
        if (sizes.size() >= 2)
        {
            sizes[0] = sizeHint.height();
            if (sizes[1] == 0) // Can be true on first call.
                sizes[1] = Max(0, nHeight - sizes[0]);
            m_spSplitter->setSizes(sizes);
        }
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onGraphEnableCheckboxToggled(const bool b)
{
    privForEachDataSource([=](GraphDataSource& ds)
    {
        ds.enable(b);
    });
}

bool DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setChartControls(QString s)
{
    auto pDefWidget = getDefinitionWidget();
    if (pDefWidget && pDefWidget->m_spRawTextDefinition)
    {
        pDefWidget->m_spRawTextDefinition->setPlainText(s);
        refresh();
        return true;
    }
    else
        return false;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setChartingEnabledState(const bool b)
{
    auto pControlPanel = getChartControlPanel();
    if (pControlPanel)
        pControlPanel->setEnabledFlag(b);
}

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::defaultSourceIdImpl() const -> GraphDataSourceId 
{
    return this->m_sDefaultDataSource;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::clearCachesImpl()
{
    m_spCache.reset();
}

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::chart() -> ChartCanvas*
{
    return (m_spGraphDisplay) ? m_spGraphDisplay->chart() : nullptr;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshImpl()
{
    DFG_OPAQUE_REF().m_refreshTimer = ::DFG_MODULE_NS(time)::TimerCpu();

    if (m_spCache)
        m_spCache->removeInvalidCaches();

    auto pChart = this->chart();
    if (!pChart)
    {
        DFG_ASSERT_WITH_MSG(false, "Internal error, no chart object available");
        return;
    }
    auto& rChart = *pChart;

    DFG_QT_CHART_CONSOLE_INFO(tr("-------- Refresh number %1 started").arg(DFG_OPAQUE_REF().m_nRefreshCounter));
    DFG_OPAQUE_REF().m_nRefreshCounter++;

    auto pControlPanel = getChartControlPanel();
    if (pControlPanel)
    {
        int nTotalSteps = -1;
        auto pDefWidget = getDefinitionWidget();
        if (pDefWidget)
            nTotalSteps = saturateCast<int>(pDefWidget->getChartDefinition().getForEachEntryCount());
        pControlPanel->onDisplayStateChanged(ChartDisplayState(ChartDisplayState::updating, 0, nTotalSteps));
    }

    // Note: beginUpdateState() may freeze GUI for a while so should be called after control panel handling which forces repaint
    //       so that user sees that something happened by clicking Apply-button.
    rChart.beginUpdateState();   

    // Clearing existing objects.
    rChart.removeAllChartObjects(false); // false = no repaint
    startChartDataFetching();
}

bool ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshTerminateImpl()
{
    auto pPreparator = DFG_OPAQUE_REF().m_spChartPreparator.data();
    if (pPreparator)
    {
        pPreparator->terminatePreparation();
        DFG_QT_CHART_CONSOLE_INFO(tr("Terminated chart update"));
        return true;
    }
    else
        return false;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onChartDataPreparationUpdate(EntryPreparedParam param)
{
    auto pControlPanel = getChartControlPanel();
    if (pControlPanel)
        pControlPanel->onDisplayStateChanged(ChartDisplayState(ChartDisplayState::updating, param.totalPrepareReadyCount(), param.totalPrepareCount()));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onChartDataPreparationReady(ChartRefreshParamPtr spParam)
{
    using namespace ::DFG_MODULE_NS(charts);

    auto updateStatusResetter = makeScopedCaller([]() { QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                         [&]() { m_bRefreshInProgress = false; QGuiApplication::restoreOverrideCursor(); });

    if (!spParam)
        return;

    auto& param = *spParam;

    auto pChart = this->chart();
    if (!pChart)
    {
        DFG_ASSERT_WITH_MSG(false, "Internal error, no chart object available");
        return;
    }
    auto& rChart = *pChart;

    GraphDefinitionEntry globalConfigEntry; // Stores global config entry if present.
    GraphDefinitionEntry* pGlobalConfigEntry = nullptr; // Set to point to global config if such exists. This and globalConfigEntry are nothing but inconvenient way to do what optional would provide.
    ::DFG_MODULE_NS(cont)::MapVectorAoS<StringUtf8, GraphDefinitionEntry> mapPanelIdToConfig;
    RefreshContext context;

    // Getting a reference to the cache object; this is important so that items that have no prepared data can use cache if available.
    // Note that this is needed because 'this' moves out cache object to refresh param.
    this->m_spCache = std::move(param.cache());

    // Preparation might have added on-demand sources so copying them back.
    this->m_dataSources = std::move(param.dataSources());

    auto pControlPanel = getChartControlPanel();
    if (pControlPanel)
        pControlPanel->onDisplayStateChanged(ChartDisplayState::finalizing);

    // Going through every item in definition entry table and redrawing them.
    param.chartDefinition().forEachEntry([&](const GraphDefinitionEntry& defEntry)
    {
        const auto sErrorString = defEntry.fieldValueStr(ChartObjectFieldIdStr_errorString);
        if (!sErrorString.empty())
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                defEntry.log(GraphDefinitionEntry::LogLevel::error, viewToQString(sErrorString));
        }

        if (!defEntry.isEnabled())
            return;

        const auto sEntryType = defEntry.graphTypeStr();

        bool bUnknownType = false;
        forEachUnrecognizedPropertyId(defEntry, [&](StringViewUtf8 svId)
        {
            if (svId == SzPtrUtf8(ChartObjectFieldIdStr_type))
            {
                bUnknownType = true;
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                    defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("unknown type '%1'").arg(untypedViewToQStringAsUtf8(sEntryType)));
            }
            else
            {
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                    defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("unknown property '%1'").arg(viewToQString(svId)));
            }
        });
        if (bUnknownType)
            return;

        // Handling case type == global_config
        if (sEntryType == ChartObjectChartTypeStr_globalConfig)
        {
            globalConfigEntry = defEntry;
            pGlobalConfigEntry = &globalConfigEntry;
            // If global_config entry has log_level, setting global log level.
            if (globalConfigEntry.hasField(ChartObjectFieldIdStr_logLevel))
            {
                // Note: the global log level is in effect until changed, i.e. it will not reset when this entry list has been run.
                DFG_DETAIL_NS::gConsoleLogHandle.setDesiredLevel(globalConfigEntry.logLevel());
                DFG_QT_CHART_CONSOLE_DEBUG(tr("Setting log level to %1").arg(viewToQString(globalConfigEntry.fieldValueStr(ChartObjectFieldIdStr_logLevel))));
            }
            return;
        }

        // Handling case type == panel_config
        if (sEntryType == ChartObjectChartTypeStr_panelConfig)
        {
            handlePanelProperties(rChart, defEntry);
            mapPanelIdToConfig[defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId)] = defEntry;
            return;
        }

        const auto configParamCreator = [&]()
        {
            auto iter = mapPanelIdToConfig.find(defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId));
            return ChartConfigParam((iter != mapPanelIdToConfig.end()) ? &iter->second : nullptr, pGlobalConfigEntry);
        };

        this->forDataSource(defEntry.sourceId(this->m_sDefaultDataSource), [&](GraphDataSource& source)
        {
            // Checking if source is ok (e.g. if file source has readable file)
            if (!source.refreshAvailability())
            {
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                    defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("Source '%1' is unavailable, source status: '%2'").arg(source.uniqueId(), source.statusDescription()));
                return;
            }

            // Checking that source type in compatible with graph type
            const auto dataType = source.dataType();
            if (dataType != GraphDataSourceType_tableSelection) // Currently only one type is supported.
            {
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                    defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("Unknown source type"));
                return;
            }

            // Note: there is similar selection logics in GraphControlAndDisplayWidget::prepareData() so if needing changes here, might need to be reflected there as well.
            auto pPreparedData = param.preparedDataForEntry(defEntry);
            ChartData dummy;
            if (param.wasTerminated() && pPreparedData == nullptr)
                pPreparedData = &dummy; // Setting pPreparedData to dummy data to prevent refreshX calls from starting to compute data.
            try
            {
                if (sEntryType == ChartObjectChartTypeStr_xy || sEntryType == ChartObjectChartTypeStr_txy || sEntryType == ChartObjectChartTypeStr_txys)
                    refreshXy(context, rChart, configParamCreator, source, defEntry, pPreparedData);
                else if (sEntryType == ChartObjectChartTypeStr_histogram)
                    refreshHistogram(context, rChart, configParamCreator, source, defEntry, pPreparedData);
                else if (sEntryType == ChartObjectChartTypeStr_bars)
                    refreshBars(context, rChart, configParamCreator, source, defEntry, pPreparedData);
                else
                {
                    if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                        defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("missing handler for type '%1'").arg(sEntryType.c_str()));
                    return;
                }
            }
            catch (const std::bad_alloc& e)
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Creating chart object for entry '%1' failed due to memory allocation failure, there may be more chart data than what can be handled. bad_alloc.what() = '%2'").arg(defEntry.index()).arg(e.what()));
            }
        });
    });

    rChart.optimizeAllAxesRanges();

    // Legend handling
    if (ChartConfigParam(pGlobalConfigEntry).value(ChartObjectFieldIdStr_showLegend, pChart->isLegendEnabled()))
    {
        pChart->enableLegend(true);
        pChart->createLegends();
    }
    else
        pChart->enableLegend(false);

    // Background handling
    pChart->setBackground(ChartConfigParam(pGlobalConfigEntry).valueStr(ChartObjectFieldIdStr_background));

    auto& timer = DFG_OPAQUE_REF().m_refreshTimer;

    DFG_MODULE_NS(time)::TimerCpu timerRepaint;
    rChart.repaintCanvas();
    const auto elapsedRepaint = timerRepaint.elapsedWallSeconds();
    const auto elapsed = timer.elapsedWallSeconds();
    DFG_QT_CHART_CONSOLE_INFO(tr("Refresh lasted %1 ms (repaint took %2 %)").arg(round<int>(1000 * elapsed)).arg(round<int>(100.0 * elapsedRepaint / elapsed)));

    // Cache diagnostics
    if (m_spCache && pControlPanel && pControlPanel->m_bLogCacheDiagnosticsOnUpdate)
    {
        QString s;
        for (const auto& item : m_spCache->m_tableSelectionDatas)
        {
            if (!item.second || !item.second->isValid()) // Not printing invalid caches.
                continue;
            if (!s.isEmpty())
                s += ", ";
            s += QString("{ key: %1, column count: %2 }").arg(item.first).arg(item.second->columnCount());
        }
        DFG_QT_CHART_CONSOLE_INFO(QString("Cache usage: %1").arg(s));
    }

    if (pControlPanel)
        pControlPanel->onDisplayStateChanged(ChartDisplayState::idle);
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::startChartDataFetching()
{
    auto pDefWidget = getDefinitionWidget();
    if (!pDefWidget)
    {
        DFG_QT_CHART_CONSOLE_ERROR("Internal error: missing control widget");
        return;
    }

    // Creating data preparation thread if it doesn't already exist
    if (!DFG_OPAQUE_REF().m_spThreadDataPreparation)
    {
        static bool bIsMetaTypeRegistered = false;
        if (!bIsMetaTypeRegistered)
        {
            qRegisterMetaType<EntryPreparedParam>("EntryPreparedParam"); // Note: format matters, e.g. may break if string has namespace qualifiers.
            qRegisterMetaType<ChartRefreshParamPtr>("ChartRefreshParamPtr"); // Note: format matters, e.g. may break if string has namespace qualifiers.
            bIsMetaTypeRegistered = true;
        }
        auto& opaqueThis = DFG_OPAQUE_REF();
        opaqueThis.m_spThreadDataPreparation = new QThread; // Note: not having 'this' as parent to prevent threading issues when deleting.
        opaqueThis.m_spThreadDataPreparation->setObjectName("chartDataPreration"); // Sets thread name visible to debugger.
        opaqueThis.m_spChartPreparator = new ChartDataPreparator;
        opaqueThis.m_spChartPreparator->moveToThread(opaqueThis.m_spThreadDataPreparation);
        DFG_QT_VERIFY_CONNECT(connect(this, &GraphControlAndDisplayWidget::sigChartDataPreparationNeeded, opaqueThis.m_spChartPreparator.data(), &ChartDataPreparator::prepareData));
        DFG_QT_VERIFY_CONNECT(connect(opaqueThis.m_spChartPreparator.data(), &GraphControlAndDisplayWidget::ChartDataPreparator::sigOnEntryPrepared, this, &GraphControlAndDisplayWidget::onChartDataPreparationUpdate));
        DFG_QT_VERIFY_CONNECT(connect(opaqueThis.m_spChartPreparator.data(), &GraphControlAndDisplayWidget::ChartDataPreparator::sigPreparationFinished, this, &GraphControlAndDisplayWidget::onChartDataPreparationReady));
        opaqueThis.m_spThreadDataPreparation->start();
    }
    // Creating refresh parameter and triggering data preparation. Once preparation is done, refresh will continue in onChartDataPreparationReady()
    ChartRefreshParamPtr spParam(new ChartRefreshParam(pDefWidget->getChartDefinition(), this->m_dataSources, std::move(m_spCache)));
    Q_EMIT sigChartDataPreparationNeeded(std::move(spParam));
}

namespace
{
    template <class ValueCont_T>
    std::vector<bool> createIncludeMask(const ::DFG_MODULE_NS(cont)::IntervalSet<int>* pIntervals, const ValueCont_T& rowValues)
    {
        using namespace DFG_ROOT_NS;
        using namespace DFG_MODULE_NS(math);
        if (!pIntervals)
            return std::vector<bool>();
        std::vector<bool> rv(rowValues.size(), true);
        auto iter = rv.begin();
        for (const auto& row : rowValues)
        {
            *iter++ = row >= 0 && row <= double(NumericTraits<int>::maxValue) && isIntegerValued(row) && pIntervals->hasValue(static_cast<int>(row));
        }
        return rv;
    }
}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt) { namespace
{

    // Returns
    //      -defaultValue if not set
    //      -defaultValue + 1 if defined as row index.
    //      -GraphDataSource::invalidIndex() if set but not found.
    auto getChosenColumnIndex(GraphDataSource& dataSource,
                              const decltype(ChartObjectFieldIdStr_xSource)& id,
                              const GraphDefinitionEntry& defEntry,
                              const GraphDataSource::DataSourceIndex defaultValue) -> GraphDataSource::DataSourceIndex
    {
        const auto sSource = defEntry.fieldValueStr(id, [] { return StringUtf8(); });
        if (sSource.empty())
            return defaultValue;
        if (sSource == SzPtrUtf8(ChartObjectSourceTypeStr_rowIndex))
            return defaultValue + 1;
        const ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem items(sSource);
        if (items.key() != SzPtrUtf8(ChartObjectSourceTypeStr_columnName))
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::info))
                defEntry.log(GraphDefinitionEntry::LogLevel::info, defEntry.tr("Unknown source type, got %1").arg(viewToQString(items.key())));
            return GraphDataSource::invalidIndex();
        }
        if (items.valueCount() != 1)
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::info))
                defEntry.log(GraphDefinitionEntry::LogLevel::info, defEntry.tr("Unexpected value count for %1: expected 1, got %2").arg(ChartObjectSourceTypeStr_columnName).arg(items.valueCount()));
            return GraphDataSource::invalidIndex();
        }
        const auto svColumnName = items.value(0).toStringView();
        const auto columnIndex = dataSource.columnIndexByName(svColumnName);
        if (columnIndex == dataSource.invalidIndex())
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                defEntry.log(GraphDefinitionEntry::LogLevel::warning, defEntry.tr("no column '%1' found from source").arg(viewToQString(svColumnName)));
            return GraphDataSource::invalidIndex();
        }
        return columnIndex;
    }

    template <class ColRange_T, class IsRowSpecifier_T>
    void makeEffectiveColumnIndexes(GraphDataSource::DataSourceIndex& x, GraphDataSource::DataSourceIndex& y, GraphDataSource::DataSourceIndex& z, const ColRange_T& colRange, IsRowSpecifier_T isRowIndexSpecifier)
    {
        if (colRange.empty())
            return;
        const auto indexOf = [&](auto i) { return ::DFG_MODULE_NS(alg)::indexOf(colRange, i); };
        const auto isPresent = [&](auto i) {return indexOf(i) < colRange.size(); };

        if (isRowIndexSpecifier(x) && isRowIndexSpecifier(y))
        {
            DFG_QT_CHART_CONSOLE_ERROR("Internal error: both x and y are of type row_index");
            return;
        }

        const auto nextUsableIndex = [&](const auto src) // Note: src is column index, not index of colRange.
        {
            auto n = ((indexOf(src) + 1) % colRange.size()); // Using next column as next (wrapping to beginning if src is last)...
            if (isPresent(z) && colRange[n] == z && colRange.size() >= 3) // ...unless it is already occupied by z-index. Then skipping z-index.
                n = ((indexOf(src) + 2) % colRange.size());
            return n;
        };
        const auto previousUsableIndex = [&](const auto src) // Note: src is column index, not index of colRange.
        {
            const auto indexOfSrc = indexOf(src);
            auto n = (indexOfSrc >= 1) ? indexOfSrc - 1 : colRange.size() - 1; // Using one before src-column as previous (wrapping to end if src is first)...
            if (isPresent(z) && colRange[n] == z && colRange.size() >= 3) // ...unless it is already occupied by z-index. Then skipping z-index.
                n = (indexOfSrc >= 2) ? indexOfSrc - 2 : colRange.size() - 2 - indexOfSrc;
            return n;
        };

        if (isPresent(x)) // Case: x was defined
        {
            if (!isPresent(y)) // y is not set?
            {
                if (isRowIndexSpecifier(y))
                    y = x;
                else
                    y = colRange[nextUsableIndex(x)];
            }
        }
        else if (isPresent(y)) // case: x is not defined and y defined
        {
            if (isRowIndexSpecifier(x))
                x = y;
            else
            {
                if (colRange.size() >= 2)
                    x = colRange[previousUsableIndex(y)];
                else // Case: only one column -> x = y
                    x = y;
            }
        }
        else // Case: both are either undefined or other one is row_index-specifier
        {
            const auto nDefaultXindex = (indexOf(z) != 0 || colRange.size() < 2) ? 0 : 1;
            const auto nDefaultYindex = (colRange.size() >= 2) ? nextUsableIndex(colRange[nDefaultXindex]) : nDefaultXindex;
            if (isRowIndexSpecifier(x))
            {
                y = colRange[nDefaultYindex];
                x = y;
            }
            else if (isRowIndexSpecifier(y))
            {
                x = colRange[nDefaultXindex];
                y = x;
            }
            else
            {
                //using defaults i.e.first column as x and next (if available) as y
                x = colRange[nDefaultXindex];
                y = colRange[nDefaultYindex];
            }
        }

        if (colRange.size() >= 3 && !isPresent(z) && !isRowIndexSpecifier(z))
        {
            // If there's no z-defined, taking first free one.
            for (size_t i = 0; i < 3; ++i)
            {
                if (x != colRange[i] && y != colRange[i])
                {
                    z = colRange[i];
                    break;
                }
            }
        }
    }
} } } // dfg:::qt::<unnamed>


auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::prepareData(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> ChartData
{
    const auto sEntryType = defEntry.graphTypeStr();
    // Note: this duplicates type selection logic from onChartDataPreparationReady()
    try
    {
        if (sEntryType == ChartObjectChartTypeStr_xy || sEntryType == ChartObjectChartTypeStr_txy || sEntryType == ChartObjectChartTypeStr_txys)
            return prepareDataForXy(spCache, source, defEntry);
        else if (sEntryType == ChartObjectChartTypeStr_histogram)
            return prepareDataForHistogram(spCache, source, defEntry);
        else if (sEntryType == ChartObjectChartTypeStr_bars)
            return prepareDataForBars(spCache, source, defEntry);
        else
            return ChartData();
    }
    catch (const std::bad_alloc& e)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Preparing data for entry '%1' failed due to memory allocation failure, there may be more chart data than what can be handled. bad_alloc.what() = '%2'").arg(defEntry.index()).arg(e.what()));
        return ChartData();
    }
}

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::prepareDataForXy(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> ChartData
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!spCache)
        spCache.reset(new ChartDataCache);

    std::array<DataSourceIndex, 3> columnIndexes;
    std::array<bool, 3> rowFlags;
    auto optData = spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optData || optData->columnCount() < 1)
        return ChartData();

    auto& tableData = *optData;

    const auto bNeedMetaStrings = defEntry.isType(ChartObjectChartTypeStr_txys);

    const bool bXisRowIndex = rowFlags[0];
    const bool bYisRowIndex = rowFlags[1];
    const bool bZisRowIndex = rowFlags[2];

    auto pXdata = tableData.columnDataByIndex(columnIndexes[0]);
    auto pYdata = tableData.columnDataByIndex(columnIndexes[1]);
    auto pZdata = (bNeedMetaStrings) ? tableData.columnStringsByIndex(columnIndexes[2]) : nullptr;

    if (!pXdata || !pYdata || (bNeedMetaStrings && !pZdata && !bZisRowIndex))
        return ChartData();

    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;

    // xValueMap is also used as final (x,y) table passed to series.
    // releaseOrCopy() will return either moved data or copy of it.
    // Note: if pXdata == pYdata, possible releasing of pXdata affects yData as well.
    auto xValueMap = tableData.releaseOrCopy(pXdata);
    xValueMap.setSorting(false);

    using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<int>;
    IntervalSet xRows;
    const IntervalSet* pxRowSet = (!xValueMap.empty()) ? defEntry.createXrowsSet(xRows, static_cast<int>(xValueMap.backKey() + 1)) : nullptr;
    if (pxRowSet)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::debug))
            defEntry.log(GraphDefinitionEntry::LogLevel::debug, tr("x_rows-entry defines %1 row(s)").arg(xRows.sizeOfSet()));
    }

    const auto& yValueMap = (pXdata != pYdata) ? *pYdata : xValueMap;
    auto xIter = xValueMap.cbegin();
    auto yIter = yValueMap.cbegin();
    const TableSelectionCacheItem::RowToStringMap dummyZmap;
    auto zIter = (pZdata) ? pZdata->cbegin() : dummyZmap.cbegin();
    decltype(TableSelectionCacheItem::RowToStringMap::m_valueStorage) zStrings; // Filled with final strings if needed.
    DataSourceIndex nSizeAfterRowFilter = 0;
    for (; xIter != xValueMap.cend() && yIter != yValueMap.cend() && (!pZdata || zIter != pZdata->cend());)
    {
        const auto xRow = xIter->first;
        const auto yRow = yIter->first;
        const auto zRow = (pZdata) ? zIter->first : 0;
        if (xRow < yRow)
        {
            ++xIter;
            continue;
        }
        else if (yRow < xRow)
        {
            ++yIter;
            continue;
        }
        // Getting here means that x and y are pointing to the same row. Checking z if needed
        if (pZdata)
        {
            if (zRow < xRow)
            {
                ++zIter;
                continue;
            }
            else if (zRow > xRow)
            {
                ++xIter;
                ++yIter;
                continue;
            }
        }
        // Getting means that all needed iterator are pointing to the same row. ->
        // storing (x,y) values to start of xValueMap if not filtered out.
        // If strings are needed, storing them to zStrings
        DFG_ASSERT_CORRECTNESS(xRow == yRow && (!pZdata || xRow == zRow));
        DFG_ASSERT_UB(::DFG_MODULE_NS(math)::isFloatConvertibleTo<int>(xRow));
        if (!pxRowSet || pxRowSet->hasValue(static_cast<int>(xRow)))
        {
            const auto x = (bXisRowIndex) ? xRow : xIter->second;
            const auto y = (bYisRowIndex) ? yRow : yIter->second;

            if (!::DFG_MODULE_NS(math)::isNan(x)) // Accepting point only if x is non-NaN
            {
                xValueMap.m_keyStorage[nSizeAfterRowFilter] = x;
                xValueMap.m_valueStorage[nSizeAfterRowFilter] = y;
                minMaxX(x);
                minMaxY(y);
                if (pZdata)
                    zStrings.push_back(zIter->second);
                else if (bNeedMetaStrings && bZisRowIndex)
                    zStrings.push_back(StringUtf8::fromRawString(::DFG_MODULE_NS(str)::toStrC(xRow)));
                nSizeAfterRowFilter++;
            }
        }
        ++xIter;
        ++yIter;
        if (pZdata)
            ++zIter;
    }

    xValueMap.m_keyStorage.resize(nSizeAfterRowFilter);
    xValueMap.m_valueStorage.resize(nSizeAfterRowFilter);
    DFG_ASSERT_CORRECTNESS(!pZdata || nSizeAfterRowFilter == zStrings.size());

    // Applying operations
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData;
    operationData.setDataRefs(&xValueMap.m_keyStorage, &xValueMap.m_valueStorage, ChartOperationPipeData::DataVectorRef((bNeedMetaStrings) ? &zStrings : nullptr));
    defEntry.applyOperations(operationData);

    ChartData rv;
    rv.copyOrMoveDataFrom(operationData);
    rv.m_bXisRowIndex = bXisRowIndex;
    rv.m_bYisRowIndex = bYisRowIndex;
    rv.m_columnDataTypes.push_back(tableData.columnDataType(pXdata));
    rv.m_columnDataTypes.push_back(tableData.columnDataType(pYdata));
    rv.m_columnDataTypes.push_back(ChartDataType::unknown);
    rv.m_columnNames.push_back(tableData.columnName(pXdata));
    rv.m_columnNames.push_back(tableData.columnName(pYdata));
    rv.m_columnNames.push_back(tableData.columnName(columnIndexes[2]));
    return rv;
}

namespace
{
    static DFG_ROOT_NS::uint32 getRunningIndexFor(::DFG_MODULE_NS(charts)::ChartCanvas& rChart, ::DFG_MODULE_NS(charts)::ChartObject& rObject, const ::DFG_MODULE_NS(qt)::GraphDefinitionEntry& defEntry)
    {
        auto pPanel = rChart.findPanelOfChartObject(&rObject);
        return (pPanel) ? pPanel->countOf(defEntry.graphTypeStr()) : 0;
    }

    template <class Cont_T>
    static auto elementByModuloIndex(Cont_T& cont, const size_t i) -> decltype(cont[0])
    {
        return cont[i % DFG_ROOT_NS::count(cont)];
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshXy(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData)
{
    using namespace ::DFG_MODULE_NS(charts);
    auto rawData = (pPreparedData) ? std::move(*pPreparedData) : prepareDataForXy(m_spCache, source, defEntry);

    auto pXvalues = rawData.constValuesByIndex(0);
    auto pYvalues = rawData.constValuesByIndex(1);
    const auto bXisRowIndex = rawData.m_bXisRowIndex;
    const auto bYisRowIndex = rawData.m_bYisRowIndex;

    if (!pXvalues || !pYvalues)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available"));
        return;
    }

    const char szRowIndexName[] = QT_TR_NOOP("Row number");
    const auto xType = (!bXisRowIndex) ? rawData.columnDataType(0) : ChartDataType(ChartDataType::unknown);
    const auto yType = (!bYisRowIndex) ? rawData.columnDataType(1) : ChartDataType(ChartDataType::unknown);
    const auto sXname = (!bXisRowIndex) ? rawData.columnName(0) : QString(szRowIndexName);
    const auto sYname = (!bYisRowIndex) ? rawData.columnName(1) : QString(szRowIndexName);
    auto spSeries = rChart.createXySeries(XySeriesCreationParam(-1, configParamCreator(), defEntry, xType, yType, qStringToStringUtf8(sXname), qStringToStringUtf8(sYname)));
    if (!spSeries)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("couldn't create series object"));
        return;
    }
    auto& rSeries = *spSeries;

    // Setting values to series.
    rSeries.setValues(makeRange(*pXvalues), makeRange(*pYvalues));
    if (defEntry.isType(::DFG_MODULE_NS(charts)::ChartObjectChartTypeStr_txys))
    {
        auto pMetaStrings = rawData.constStringsByIndex(2);
        if (pMetaStrings)
        {
            using PointMetaData = ::DFG_MODULE_NS(charts)::XySeries::PointMetaData;
            rSeries.setMetaDataByFunctor([&](const double t, const Dummy, const Dummy) -> PointMetaData
            {
                size_t n = 0;
                if (::DFG_MODULE_NS(math)::isFloatConvertibleTo(t, &n) && isValidIndex(*pMetaStrings, n))
                    return PointMetaData::fromStableStringView((*pMetaStrings)[n]);
                else
                    return PointMetaData();
            });
        }
    }

    // Setting line style
    {
        bool bFound = false;
        defEntry.doForLineStyleIfPresent([&](const char* psz) { bFound = true; rSeries.setLineStyle(psz); });
        if (!bFound)
        {
            if (defEntry.isType(ChartObjectChartTypeStr_txys)) // For txys, always using none-style as default.
                rSeries.setLineStyle(ChartObjectLineStyleStr_none);
            else // For others types, if not found, setting value from default config or if not present there either, using basic style
                rSeries.setLineStyle(configParamCreator().valueStr(ChartObjectFieldIdStr_lineStyle, SzPtrUtf8(ChartObjectLineStyleStr_basic)));
        }
    }

    // Setting point style
    {
        bool bFound = false;
        defEntry.doForPointStyleIfPresent([&](const char* psz) { bFound = true; rSeries.setPointStyle(psz); });
        if (!bFound) // If not found, setting value from default config or if not present there either, using basic-style
            rSeries.setPointStyle(configParamCreator().valueStr(ChartObjectFieldIdStr_pointStyle, SzPtrUtf8(ChartObjectPointStyleStr_basic)));
    }

    setCommonChartObjectProperties(context, rSeries, defEntry, configParamCreator, DefaultNameCreator("Graph", getRunningIndexFor(rChart, rSeries, defEntry), sXname, sYname));
}

namespace
{
    template <class Cont_T>
    Cont_T filterByXrows(const Cont_T& rowValueMap, const ::DFG_MODULE_NS(cont)::IntervalSet<int>& xRows)
    {
        Cont_T filtered;
        for (const auto& kv : rowValueMap)
        {
            if (xRows.hasValue(static_cast<int>(kv.first)))
            {
                filtered.m_keyStorage.push_back(kv.first);
                filtered.m_valueStorage.push_back(kv.second);
            }
        }
        return filtered;
    }

    template <class Cont_T>
    bool handleXrows(const ::DFG_MODULE_NS(qt)::GraphDefinitionEntry& defEntry, const Cont_T*& pRowToItemMap, Cont_T& rowToItemMapCopy)
    {
        ::DFG_MODULE_NS(cont)::IntervalSet<int> xRows;
        using namespace ::DFG_MODULE_NS(qt);
        if (!pRowToItemMap->empty() && defEntry.createXrowsSet(xRows, static_cast<int>(pRowToItemMap->backKey() + 1)))
        {
            rowToItemMapCopy = filterByXrows(*pRowToItemMap, xRows);
            pRowToItemMap = &rowToItemMapCopy;
        }

        if (pRowToItemMap->valueRange().size() < 1 && defEntry.isType(ChartObjectChartTypeStr_histogram))
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                defEntry.log(GraphDefinitionEntry::LogLevel::error, defEntry.tr("too few points (%1) for histogram").arg(pRowToItemMap->valueRange().size()));
            return false;
        }
        return true;
    }
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::prepareDataForHistogram(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> ChartData
{
    using namespace ::DFG_MODULE_NS(charts);
    const auto nColumnCount = source.columnCount();
    if (nColumnCount < 1)
        return ChartData();

    if (!spCache)
        spCache.reset(new ChartDataCache);

    const auto sBinType = defEntry.fieldValueStr(ChartObjectFieldIdStr_binType, [] { return StringUtf8(DFG_UTF8("number")); });

    if (sBinType != DFG_UTF8("number") && sBinType != DFG_UTF8("text"))
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("Unrecognized bin_type '%1', using default").arg(sBinType.c_str().c_str()));
    }

    const bool bTextValued = (sBinType == DFG_UTF8("text"));

    std::array<DataSourceIndex, 3> columnIndexes;
    std::array<bool, 3> rowFlags;
    auto optTableData = spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optTableData)
        return ChartData();

    ChartObjectHolder<Histogram> spHistogram;
    if (!bTextValued)
    {
        if (!optTableData || optTableData->columnCount() < 1)
            return ChartData();

        auto pCacheColumn = optTableData->columnDataByIndex(columnIndexes[0]);

        if (!pCacheColumn)
            return ChartData();

        TableSelectionCacheItem::RowToValueMap singleColumnCopy;
        const TableSelectionCacheItem::RowToValueMap* pRowToValues = pCacheColumn;

        if (!handleXrows(defEntry, pRowToValues, singleColumnCopy))
            return ChartData();

        ValueVectorD xVals;
        ValueVectorD yVals;

        // Creating histogram bin values
        {
            // Checking input value range and returning early if there is no valid range available (e.g. only NaN)
            auto valueRange = makeRange(pRowToValues->m_valueStorage);
            auto minMaxPair = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(valueRange);
            if (*minMaxPair.first > *minMaxPair.second || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.first) || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.second))
                return ChartData();

            const bool bOnlySingleValue = (*minMaxPair.first == *minMaxPair.second);
            const auto nBinCount = (!bOnlySingleValue) ? defEntry.fieldValue<int>(ChartObjectFieldIdStr_binCount, 100) : -1;

            if (nBinCount >= 0)
            {
#if (BOOST_VERSION >= 107000) // If have boost histogram
                try
                {
                    // Creating histogram points using boost::histogram.
                    // Adding small adjustment to upper boundary so that items identical to max value won't get excluded from histogram.
                    const auto binWidth = (*minMaxPair.second - *minMaxPair.first) / static_cast<double>(nBinCount);
                    const auto edgeAdjustment = 0.001 * binWidth;
                    auto hist = boost::histogram::make_histogram(boost::histogram::axis::regular<>(static_cast<unsigned int>(nBinCount), *minMaxPair.first, *minMaxPair.second + edgeAdjustment, "x"));
                    std::for_each(valueRange.begin(), valueRange.end(), std::ref(hist));

                    for (auto&& x : indexed(hist, boost::histogram::coverage::all))
                    {
                        const double xVal = x.bin().center();
                        if (DFG_MODULE_NS(math)::isNan(xVal) || DFG_MODULE_NS(math)::isInf(xVal))
                            continue;
                        xVals.push_back(xVal);
                        yVals.push_back(*x);
                    }
                }
                catch (const std::exception& e)
                {
                    // Failed to create histogram.
                    if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                        defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("Failed to create histogram, exception message: '%1'").arg(e.what()));
                    return ChartData();
                }
#else // If not boost::histogram available
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                    defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("Histograms are not available, requires building with Boost >= 1.70"));
                return ChartData();
#endif // "If have boost histogram"
            }
            else // Case: bin for every value.
            {
                ::DFG_MODULE_NS(cont)::MapVectorSoA<double, double> countsPerValue;
                for (const auto& val : valueRange) if (!::DFG_MODULE_NS(math)::isNan(val))
                {
                    auto insertRv = countsPerValue.insert(val, 1);
                    if (!insertRv.second) // If already existed, adding 1 to existing count.
                        insertRv.first->second += 1;
                }
                xVals = std::move(countsPerValue.m_keyStorage);
                yVals = std::move(countsPerValue.m_valueStorage);
            }
        } // End creating histogram bin values.

        // Applying operations
        // Note: operations semantics changed from dfgQtTableEditor 1.9.0 to 2.0.0: in 1.9.0 only x was available and it referred to raw input values,
        //       in 2.0.0 both (x, y) are available and x values are bin positions, not raw input values. y values are bar heights.
        ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&xVals, &yVals);
        defEntry.applyOperations(operationData);

        auto pValues = operationData.constValuesByIndex(0);
        if (!pValues || pValues->empty())
        {
            if (!pValues && defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available after operations"));
            return ChartData();
        }

        ChartData rv;
        rv.copyOrMoveDataFrom(operationData);
        rv.m_columnDataTypes.push_back(optTableData->columnDataType(pCacheColumn));
        rv.m_columnNames.push_back(optTableData->columnName(pCacheColumn));
        return rv;

    }
    else // Case text valued
    {
        auto pStrings = optTableData->columnStringsByIndex(columnIndexes[0]);
        if (!pStrings || pStrings->empty())
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("no data found for histogram"));
            return ChartData();
        }

        TableSelectionCacheItem::RowToStringMap stringColumnCopy;

        if (!handleXrows(defEntry, pStrings, stringColumnCopy))
            return ChartData();


        // Creating y-values
        ::DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, double, ::DFG_MODULE_NS(cont)::Vector<StringUtf8>, ::DFG_MODULE_NS(cont)::ValueVector<double>> counts;
        {
            for (const auto& kv : *pStrings)
                counts[kv.second]++;
        }

        // Applying operations
        ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&counts.m_keyStorage, &counts.m_valueStorage);
        defEntry.applyOperations(operationData);

        auto pFinalStrings = operationData.constStringsByIndex(0);
        if (!pFinalStrings || pFinalStrings->empty())
        {
            if (!pStrings && defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available after operations"));
            return ChartData();
        }

        ChartData rv;
        rv.copyOrMoveDataFrom(operationData);
        rv.m_columnNames.push_back(optTableData->columnName(columnIndexes[0]));
        return rv;
    }

}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshHistogram(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData)
{
    using namespace ::DFG_MODULE_NS(charts);

    auto rawData = (pPreparedData) ? std::move(*pPreparedData) : prepareDataForHistogram(m_spCache, source, defEntry);

    const auto sXaxisName = rawData.columnName(0);
    ChartObjectHolder<Histogram> spHistogram;
    auto pCounts = rawData.constValuesByIndex(1);
    if (rawData.stringsByIndex(0) == nullptr)
    {
        const auto xType = rawData.columnDataType(0);
        auto pValues = rawData.constValuesByIndex(0);
        if (pValues && pCounts)
            spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, makeRange(*pValues), makeRange(*pCounts), xType, qStringToStringUtf8(sXaxisName)));
    }
    else // Case text valued
    {
        auto pStrings = rawData.constStringsByIndex(0);
        if (pStrings && pCounts)
            spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, makeRange(*pStrings), makeRange(*pCounts), qStringToStringUtf8(sXaxisName)));
    }

    if (!spHistogram)
        return;
    setCommonChartObjectProperties(context, *spHistogram, defEntry, configParamCreator, DefaultNameCreator("Histogram", getRunningIndexFor(rChart, *spHistogram, defEntry)));
}

auto ::DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::prepareDataForBars(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> ChartData
{
    using namespace ::DFG_MODULE_NS(charts);

    const auto nColumnCount = source.columnCount();
    if (nColumnCount < 1)
        return ChartData();

    if (!spCache)
        spCache.reset(new ChartDataCache);

    std::array<DataSourceIndex, 3> columnIndexes;
    std::array<bool, 3> rowFlags;
    auto optTableData = spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optTableData || optTableData->columnCount() < 1)
        return ChartData();

    auto pFirstCol = optTableData->columnStringsByIndex(columnIndexes[0]);
    auto pSecondCol = optTableData->columnDataByIndex(columnIndexes[1]);

    if (!pFirstCol || !pSecondCol)
        return ChartData();

    TableSelectionCacheItem::RowToStringMap labelColumnCopy;
    TableSelectionCacheItem::RowToValueMap valueColumnCopy;
    // Applying x_rows filter if defined
    {
        if (!handleXrows(defEntry, pFirstCol, labelColumnCopy))
            return ChartData();
        if (!handleXrows(defEntry, pSecondCol, valueColumnCopy))
            return ChartData();
    }

    // Applying operations
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&pFirstCol->m_valueStorage, &pSecondCol->m_valueStorage);
    defEntry.applyOperations(operationData);

    ChartData rawData;
    rawData.copyOrMoveDataFrom(operationData);
    rawData.m_columnNames.push_back(optTableData->columnName(columnIndexes[0]));
    rawData.m_columnNames.push_back(optTableData->columnName(columnIndexes[1]));
    return rawData;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshBars(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData)
{
    using namespace ::DFG_MODULE_NS(charts);

    auto rawData = (pPreparedData) ? std::move(*pPreparedData) : prepareDataForBars(m_spCache, source, defEntry);

    auto pStrings = rawData.constStringsByIndex(0);
    auto pValues = rawData.constValuesByIndex(1);
    if (!pStrings || !pValues)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no data available"));
        return;
    }

    const auto sXaxisName = rawData.columnName(0);
    const auto sYaxisName = rawData.columnName(1);

    auto barSeries = rChart.createBarSeries(BarSeriesCreationParam(configParamCreator(), defEntry, makeRange(*pStrings), makeRange(*pValues), ChartDataType::unknown,
                                                                     qStringToStringUtf8(sXaxisName), qStringToStringUtf8(sYaxisName)));

    const auto nFirstIndex = (!barSeries.empty() && barSeries[0] != nullptr) ? getRunningIndexFor(rChart, *barSeries[0], defEntry) - barSeries.size() : 0;
    uint32 nIndexOffset = 0;
    for (auto& spBarSeries : barSeries)
    {
        if (spBarSeries)
        {
            auto sName = spBarSeries->name();
            auto defaultNameCreator = (!sName.empty()) ? DefaultNameCreator(sName.release()) : DefaultNameCreator("Bars", -1);
            defaultNameCreator.m_nIndex = saturateCast<int>(1u + nFirstIndex + nIndexOffset);
            setCommonChartObjectProperties(context, *spBarSeries, defEntry, configParamCreator, defaultNameCreator);
            ++nIndexOffset;
        }
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setCommonChartObjectProperties(RefreshContext& context, ChartObject& rObject, const GraphDefinitionEntry& defEntry, ConfigParamCreator configParamCreator, const DefaultNameCreator& defaultNameCreator)
{
    DFG_UNUSED(context);
    using namespace ::DFG_MODULE_NS(charts);

    //const char* defaultColours[] = { "red",     "blue",    "black",   "cyan",    "magenta",  "green",   "orange" };
    const char* defaultColours[] =   { "#ff0000", "#0000ff", "#000000", "#00ffff", "#ff00ff",  "#008000", "#ffa500" };

    rObject.setName(defEntry.fieldValueStr(ChartObjectFieldIdStr_name, defaultNameCreator));
    const auto defaultColour = SzPtrUtf8(elementByModuloIndex(defaultColours, defaultNameCreator.m_nIndex));
    const auto defaultLineColour = [&] { return configParamCreator().valueStr(ChartObjectFieldIdStr_lineColour, defaultColour); };
    rObject.setLineColour(defEntry.fieldValueStr(ChartObjectFieldIdStr_lineColour, defaultLineColour));
    const auto objectType = rObject.type();
    if (objectType == ChartObjectType::histogram || objectType == ChartObjectType::barSeries)
    {
        const auto defaultFillColour = [&] { return configParamCreator().valueStr(ChartObjectFieldIdStr_fillColour, StringUtf8::fromRawString(format_fmt("#1E{}", defaultColour.c_str() + 1))); };
        rObject.setFillColour(defEntry.fieldValueStr(ChartObjectFieldIdStr_fillColour, defaultFillColour));
    }
    else
        rObject.setFillColour(defEntry.fieldValueStr(ChartObjectFieldIdStr_fillColour));

}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::handlePanelProperties(ChartCanvas& rChart, const GraphDefinitionEntry& defEntry)
{
    const auto sPanelId = defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId);

    // Handling axis properties
    defEntry.forEachPropertyIdStartingWith(ChartObjectFieldIdStr_axisProperties, [&](const StringViewUtf8& svKey, const StringViewUtf8& idPart)
    {
        rChart.setAxisProperties(sPanelId, idPart, ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem::fromStableView(defEntry.fieldValueStr(svKey)));
    });

    const auto sAxisLabelColour = defEntry.fieldValueStr(ChartObjectFieldIdStr_axisLabelColour);

    rChart.setPanelTitle(sPanelId, defEntry.fieldValueStr(ChartObjectFieldIdStr_title), sAxisLabelColour);

    rChart.setAxisLabel(sPanelId, DFG_UTF8("x"), defEntry.fieldValueStr(ChartObjectFieldIdStr_xLabel));
    rChart.setAxisLabel(sPanelId, DFG_UTF8("y"), defEntry.fieldValueStr(ChartObjectFieldIdStr_yLabel));
    rChart.setAxisLabel(sPanelId, DFG_UTF8("y2"), defEntry.fieldValueStr(ChartObjectFieldIdStr_y2Label));

    rChart.setAxisTickLabelDirection(sPanelId, DFG_UTF8("x"), defEntry.fieldValueStr(ChartObjectFieldIdStr_xTickLabelDirection));
    rChart.setAxisTickLabelDirection(sPanelId, DFG_UTF8("y"), defEntry.fieldValueStr(ChartObjectFieldIdStr_yTickLabelDirection));

    // Axis and label colours
    if (!sAxisLabelColour.empty())
    {
        const QColor colour(viewToQString(sAxisLabelColour));
        if (colour.isValid())
        {
            // Axis colours
            rChart.setPanelAxesColour(sPanelId, sAxisLabelColour);
            // Label colours
            rChart.setPanelAxesLabelColour(sPanelId, sAxisLabelColour);
        }
        else
            DFG_QT_CHART_CONSOLE_WARNING(tr("Unable to parse colour '%1', colour not set for panel '%2'").arg(viewToQString(sAxisLabelColour), viewToQString(sPanelId)));
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::addDataSource(std::unique_ptr<GraphDataSource> spSource)
{
    if (!spSource)
        return;

    // Brittle hack with connection types: when sources are signaling changes to caches with directConnection, having this connection as queued connection
    // allows caches to invalidate themselves before onDataSourceChanged() gets handled.
    DFG_QT_VERIFY_CONNECT(connect(spSource.get(), &GraphDataSource::sigChanged, this, &GraphControlAndDisplayWidget::onDataSourceChanged, Qt::QueuedConnection));

    DFG_QT_VERIFY_CONNECT(connect(spSource.get(), &QObject::destroyed, this, &GraphControlAndDisplayWidget::onDataSourceDestroyed));

    auto pDefWidget = getDefinitionWidget();
    if (pDefWidget)
        spSource->setChartDefinitionViewer(pDefWidget->getChartDefinitionViewer());
    else
        DFG_QT_CHART_CONSOLE_ERROR("Internal error: missing definition widget");

    m_dataSources.m_sources.push_back(std::shared_ptr<GraphDataSource>(spSource.release()));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setDefaultDataSourceId(const GraphDataSourceId& sDefaultDataSource)
{
    m_sDefaultDataSource = sDefaultDataSource;
}

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::getDefinitionWidget() -> GraphDefinitionWidget*
{
    return (m_spControlPanel) ? m_spControlPanel->findChild<GraphDefinitionWidget*>() : nullptr;
}

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::getDefinitionWidget() const -> const GraphDefinitionWidget*
{
    return (m_spControlPanel) ? m_spControlPanel->findChild<const GraphDefinitionWidget*>() : nullptr;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onDataSourceChanged()
{
    if (m_bRefreshPending)
        return;

    if (m_spControlPanel && !m_spControlPanel->getEnabledFlag())
    {
        DFG_QT_CHART_CONSOLE_DEBUG("Chart is disable, ignoring data source change notification");
        return;
    }

    // Hack: accessing through sender() for now (onDataSourceChanged() probably needs params)
    auto pSenderSource = qobject_cast<const GraphDataSource*>(sender());

    if (!pSenderSource)
    {
        DFG_QT_CHART_CONSOLE_WARNING("Internal bug: received change signal from unknown data source.");
        return;
    }

    auto pDefWidget = getDefinitionWidget();
    if (!pDefWidget)
    {
        DFG_QT_CHART_CONSOLE_ERROR("Internal error: missing definition widget");
        return;
    }

    bool bHasErrorEntries = false;
    if (!pDefWidget->getChartDefinition().isSourceUsed(pSenderSource->uniqueId(), &bHasErrorEntries))
    {
        // Ignoring signal only if there no error entries: user might have meant this source to be used so
        // don't want to skip possible error message from invalid json.
        if (!bHasErrorEntries)
        {
            DFG_QT_CHART_CONSOLE_DEBUG("Received change signal from unused source -> ignoring refresh.");
            return;
        }
    }

    m_bRefreshPending = true;
    QTimer::singleShot(0, this, [&]() { refresh(); m_bRefreshPending = false; });
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onDataSourceDestroyed()
{
    //refresh();
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)> func)
{
    auto iter = m_dataSources.findById(id);

    // If source wasn't found, checking if it's file source and creating new source if needed.
    if (iter == m_dataSources.end())
    {
        const auto sId = qStringToStringUtf8(id);
        iter = tryCreateOnDemandDataSource(id, m_dataSources);
    }

    if (iter != m_dataSources.end())
        func(**iter);
    else
        DFG_QT_CHART_CONSOLE_WARNING(tr("Didn't find or couldn't create data source '%1', available sources (%2 in total): { %3 }")
            .arg(id)
            .arg(m_dataSources.size())
            .arg(m_dataSources.idListAsString()));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::privForEachDataSource(std::function<void(GraphDataSource&)> func)
{
    DFG_MODULE_NS(alg)::forEachFwd(m_dataSources.m_sources, [&](const std::shared_ptr<GraphDataSource>& sp)
    {
        if (sp)
            func(*sp);
    });
}

::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::SourceSpanBuffer(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ColumnDataTypeMap* pColumnDataTypeMap, QueryCallback func)
    : m_nColumn(nCol)
    , m_queryDetails(queryDetails)
    , m_pColumnDataTypeMap(pColumnDataTypeMap)
    , m_queryCallback(func)
{
    m_stringBuffer.reserveContentStorage_byBaseCharCount(contentBlockSize() + 500);
}

::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::~SourceSpanBuffer() = default;

void ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::storeToBuffer(const DataSourceIndex nRow, const StringViewUtf8& sv)
{
    m_stringBuffer.insert(nRow, sv);
    if (m_stringBuffer.contentStorageSize() > contentBlockSize())
        submitData();
}

void ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::storeToBuffer(const DataSourceIndex nRow, const QVariant& var)
{
    const auto s = var.toString();
    auto utfRange = makeSzRange(s.utf16());
    m_stringBuffer.insertRaw(nRow, [&](decltype(m_stringBuffer)::InsertIterator iter) { ::DFG_MODULE_NS(utf)::utf16To8(utfRange, iter); return true; });
    if (m_stringBuffer.contentStorageSize() > contentBlockSize())
        submitData();
}

void ::DFG_MODULE_NS(qt)::DFG_DETAIL_NS::SourceSpanBuffer::submitData()
{
    if (m_stringBuffer.empty())
        return;
    const bool bAreRowsNeeded = m_queryDetails.areRowsRequested();
    const bool bAreNumbersNeeded = m_queryDetails.areNumbersRequested();
    const bool bAreStringsNeeded = m_queryDetails.areStringsRequested();
    const auto nCount = m_stringBuffer.size();
    if (bAreRowsNeeded)
        m_rowBuffer.resize(nCount);
    if (bAreNumbersNeeded)
        m_valueBuffer.resize(nCount);
    if (bAreStringsNeeded)
        m_stringViewBuffer.resize(nCount);

    auto iterRow = m_rowBuffer.begin();
    auto iterValue = m_valueBuffer.begin();
    auto iterViewBuffer = m_stringViewBuffer.begin();
    for (const auto& item : m_stringBuffer)
    {
        if (bAreRowsNeeded)
            *iterRow++ = static_cast<double>(item.first);
        const auto sv = item.second(m_stringBuffer);
        if (bAreNumbersNeeded)
            *iterValue++ = GraphDataSource::cellStringToDouble(sv, m_nColumn, m_pColumnDataTypeMap);
        if (bAreStringsNeeded)
            *iterViewBuffer++ = sv.toStringView();
    }
    SourceDataSpan dataSpan;
    if (bAreRowsNeeded)
        dataSpan.setRows(m_rowBuffer);
    if (bAreNumbersNeeded)
        dataSpan.set(m_valueBuffer);
    if (bAreStringsNeeded)
        dataSpan.set(m_stringViewBuffer);
    m_queryCallback(dataSpan);
    m_stringBuffer.clear_noDealloc();
}
