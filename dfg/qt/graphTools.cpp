#include "graphTools.hpp"

#include "connectHelper.hpp"
#include "widgetHelpers.hpp"
#include "ConsoleDisplay.hpp"
#include "ConsoleDisplay.cpp"
#include "JsonListWidget.hpp"

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

    #include <QListWidget>
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QVariantMap>
    #include <QTextStream>

DFG_END_INCLUDE_QT_HEADERS

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
        // Note: boost/histogram was introduced in 1.70 (2019-04-12)
        // Note: including boost/histogram here under QCustomPlot section since it is (at least for the time being) only needed when QCustomPlot is available;
        //       don't want to require boost 1.70 unless it is actually used.
        #include <boost/histogram.hpp>
        #include "qcustomplot/qcustomplot.h"
    #endif
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

// Including before <boost/histogram.hpp> would cause compilation error on MSVC as this includes Shlwapi.h which for unknown reason breaks histogram
#include "CsvFileDataSource.hpp"
#include "SQLiteFileDataSource.hpp"

using namespace DFG_MODULE_NS(charts)::fieldsIds;

#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
DFG_ROOT_NS_BEGIN{
    // QCPDataContainer doesn't seem to have value_type typedef, so adding specializations to ElementType<> so that QCPDataContainer works with nearestRangeInSorted()
    DFG_SUB_NS(cont)
    {
        namespace DFG_DETAIL_NS
        {
            template <> struct ElementTypeFromConstReferenceRemoved<QCPGraphDataContainer> { typedef QCPGraphData type; };
            template <> struct ElementTypeFromConstReferenceRemoved<QCPCurveDataContainer> { typedef QCPCurveData type; };
            template <> struct ElementTypeFromConstReferenceRemoved<QCPBarsDataContainer>  { typedef QCPBarsData type; };
        }
    } // module cont
} // namespace DFG_ROOT_NS
#endif // DFG_ALLOW_QCUSTOMPLOT

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
    void makeEffectiveColumnIndexes(GraphDataSource::DataSourceIndex& x, GraphDataSource::DataSourceIndex& y, const ColRange_T& colRange, IsRowSpecifier_T isRowIndexSpecifier);

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

} } } // dfg:::qt::<unnamed>

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

namespace
{

using ConsoleLogLevel = ::DFG_MODULE_NS(charts)::AbstractChartControlItem::LogLevel;


static ConsoleDisplayEntryType consoleLogLevelToEntryType(const ConsoleLogLevel logLevel)
{
    switch (logLevel)
    {
        case ConsoleLogLevel::error:   return ConsoleDisplayEntryType::error;
        case ConsoleLogLevel::warning: return ConsoleDisplayEntryType::warning;
        default:                       return ConsoleDisplayEntryType::generic;
    }
}

class ConsoleLogHandle
{
public:
    using HandlerT = std::function<void(const char*, ConsoleLogLevel)>;

    ~ConsoleLogHandle()
    {
    }

    void setHandler(HandlerT handler)
    {
        m_handler = std::move(handler);
        privSetEffectiveLevel();
    }

    void setDesiredLevel(ConsoleLogLevel newLevel)
    {
        m_desiredLevel = newLevel;
        privSetEffectiveLevel();
    }

    void log(const char* psz, const ConsoleLogLevel msgLogLevel)
    {
        if (m_handler)
            m_handler(psz, msgLogLevel);
    }

    void log(const QString& s, const ConsoleLogLevel msgLogLevel)
    {
        if (m_handler)
            m_handler(s.toUtf8(), msgLogLevel);
    }

    void privSetEffectiveLevel()
    {
        if (m_handler)
            m_effectiveLevel = m_desiredLevel;
        else
            m_effectiveLevel = ConsoleLogLevel::none;
    }

    ConsoleLogLevel effectiveLevel() const { return m_effectiveLevel; }

    ConsoleLogLevel m_effectiveLevel = ConsoleLogLevel::none;
    ConsoleLogLevel m_desiredLevel = ConsoleLogLevel::info;
    HandlerT m_handler = nullptr;
};

static ConsoleLogHandle gConsoleLogHandle;

QString formatOperationErrorsForUserVisibleText(const ::DFG_MODULE_NS(charts)::ChartEntryOperation& op)
{
    const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
    return tr("error mask = 0x%1").arg(op.m_errors, 0, 16);
}

QString getOperationDefinitionUsageGuide(const StringViewUtf8& svOperationId)
{
    using namespace ::DFG_MODULE_NS(charts)::operations;
    const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
    if (svOperationId == PassWindowOperation::id())
        return tr("%1(<axis>, <lower_bound>, <upper_bound>)").arg(viewToQString(svOperationId));
    return QString();
}

// Returns initial placeholder text for operation definition.
QString getOperationDefinitionPlaceholder(const StringViewUtf8& svOperationId)
{
    using namespace ::DFG_MODULE_NS(charts)::operations;
    const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
    if (svOperationId == PassWindowOperation::id())
        return tr("%1(x, 0, 1)").arg(viewToQString(svOperationId));
    return QString("%1()").arg(viewToQString(svOperationId));
}

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

} // unnamed namespace

#define DFG_QT_CHART_CONSOLE_LOG_NO_LEVEL_CHECK(LEVEL, MSG) gConsoleLogHandle.log(MSG, LEVEL)
#define DFG_QT_CHART_CONSOLE_LOG(LEVEL, MSG) if (LEVEL <= gConsoleLogHandle.effectiveLevel()) DFG_QT_CHART_CONSOLE_LOG_NO_LEVEL_CHECK(LEVEL, MSG)
#define DFG_QT_CHART_CONSOLE_DEBUG(MSG)      DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::debug, MSG)
#define DFG_QT_CHART_CONSOLE_INFO(MSG)       DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::info, MSG)
#define DFG_QT_CHART_CONSOLE_WARNING(MSG)    DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::warning, MSG)
#define DFG_QT_CHART_CONSOLE_ERROR(MSG)      DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::error, MSG)


void ChartController::refresh()
{
    if (m_bRefreshInProgress)
        return; // refreshImpl() already in progress; not calling it.
    auto flagResetter = makeScopedCaller([&]() { m_bRefreshInProgress = true; QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor)); },
                                         [&]() { m_bRefreshInProgress = false; QGuiApplication::restoreOverrideCursor(); });
    try
    {
        refreshImpl();
    }
    catch (const std::bad_alloc& e)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Updating chart failed due to memory allocation failure, there may be more chart data than what can be handled. bad_alloc.what() = '%1'").arg(e.what()));
    }
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

    StringUtf8 operator()() const;

    const char* m_pszType;
    const int m_nIndex;
    QString m_sXname;
    QString m_sYname;
};


DefaultNameCreator::DefaultNameCreator(const char* pszType, const int nIndex, const QString& sXname, const QString& sYname)
    : m_pszType(pszType)
    , m_nIndex(nIndex)
    , m_sXname(sXname)
    , m_sYname(sYname)
{
}

auto DefaultNameCreator::operator()() const -> StringUtf8
{
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDataSource
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


double ::DFG_MODULE_NS(qt)::GraphDataSource::dateToDouble(QDateTime&& dt)
{
    if (dt.isValid())
    {
        const auto timeSpec = dt.timeSpec();
        if (timeSpec == Qt::LocalTime)
            dt.setTimeSpec(Qt::UTC);
        return static_cast<double>(dt.toMSecsSinceEpoch()) / 1000.0;
    }
    else
        return std::numeric_limits<double>::quiet_NaN();
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::timeToDouble(const QTime& t)
{
    return (t.isValid()) ? static_cast<double>(t.msecsSinceStartOfDay()) / 1000.0 : std::numeric_limits<double>::quiet_NaN();
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::stringToDouble(const QString& s)
{
    bool b;
    const auto v = s.toDouble(&b);
    return (b) ? v : std::numeric_limits<double>::quiet_NaN();
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::stringToDouble(const StringViewSzC& sv)
{
    bool bOk;
    auto rv = ::DFG_MODULE_NS(str)::strTo<double>(sv, &bOk);
    return (bOk) ? rv : std::numeric_limits<double>::quiet_NaN();
}

double ::DFG_MODULE_NS(qt)::GraphDataSource::cellStringToDouble(const StringViewSzUtf8& sv)
{
    return cellStringToDouble(sv, GraphDataSource::invalidIndex(), nullptr);
}

// Return value in case of invalid input as GIGO, in most cases returns NaN. Also in case of invalid input typeMap's value at nCol is unspecified.
double ::DFG_MODULE_NS(qt)::GraphDataSource::cellStringToDouble(const StringViewSzUtf8& svUtf8, const DataSourceIndex nCol, ColumnDataTypeMap* pTypeMap)
{
    const auto updateColumnDataType = [&](ChartDataType t)
    {
        if (!pTypeMap)
            return;
        auto insertRv = pTypeMap->insert(nCol, t);
        if (!insertRv.second) // Already existed?
            insertRv.first->second.setIfExpands(t);
    };
    const auto dateToDoubleAndColumnTypeHandling = [&](QDateTime&& dt, ChartDataType dataTypeHint)
    {
        const auto& time = dt.time();
        if (dataTypeHint.isDateNoTzType() && time.msecsSinceStartOfDay() == 0 && dataTypeHint != ChartDataType::dateOnlyYearMonth)
            dataTypeHint = ChartDataType::dateOnly;
        else if (time.msec() == 0)
        {
            if (dataTypeHint == ChartDataType::dateAndTimeMillisecond)
                dataTypeHint = ChartDataType::dateAndTime;
            else if (dataTypeHint == ChartDataType::dateAndTimeMillisecondTz)
                dataTypeHint = ChartDataType::dateAndTimeTz;
        }
        updateColumnDataType(dataTypeHint);
        return dateToDouble(std::move(dt));
    };

    // Raw view so that can use indexing
    const StringViewC s(svUtf8.beginRaw(), svUtf8.endRaw());
    //StringViewSzC s(svUtf8.beginRaw());

    const auto viewToQString = [&](const StringViewC& sv) { return ::DFG_MODULE_NS(qt)::viewToQString(StringViewUtf8(SzPtrUtf8(sv.begin()), SzPtrUtf8(sv.end()))); };

    const auto isTzStartChar = [](const QChar& c) { return ::DFG_MODULE_NS(alg)::contains("Z+-", c.toLatin1()); };
    // TODO: add parsing for fractional part longer than 3 digits.
    if (s.size() >= 8 && s[4] == '-' && s[7] == '-') // Something starting with ????-??-?? (ISO 8601, https://en.wikipedia.org/wiki/ISO_8601)
    {
        // size 19 is yyyy-MM-ddThh:mm:ss
        if (s.size() >= 19 && s[13] == ':' && s[16] == ':' && ::DFG_MODULE_NS(alg)::contains("T ", s[10])) // Case ????-??-??[T ]hh:mm:ss[.zzz][Z|HH:00]
        {
            // size 23 is yyyy-mm-ssThh:mm:ss.zzz
            if (s.size() >= 23 && s[19] == '.')
            {
                // Timezone specifier after milliseconds?
                if (s.size() >= 24 && isTzStartChar(s[23]))
                    return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), Qt::ISODateWithMs), ChartDataType::dateAndTimeMillisecondTz);
                else
                    return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), QString("yyyy-MM-dd%1hh:mm:ss.zzz").arg(s[10])), ChartDataType::dateAndTimeMillisecond);
            }
            else if (s.size() >= 20 && isTzStartChar(s[19]))
                return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), Qt::ISODate), ChartDataType::dateAndTimeTz);
            else
                return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), QString("yyyy-MM-dd%1hh:mm:ss").arg(s[10])), ChartDataType::dateAndTime);
        }
        else if (s.size() == 13 && s[10] == ' ') // Case: "yyyy-MM-dd Wd". where Wd is two char weekday indicator.
        {
            auto sQstring = viewToQString(s);
            return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(sQstring, QString("yyyy-MM-dd'%1'").arg(sQstring.mid(10, 3))), ChartDataType::dateOnly);
        }
        else if (s.size() == 10) // Case: "yyyy-MM-dd"
            return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), "yyyy-MM-dd"), ChartDataType::dateOnly);
        else
            return std::numeric_limits<double>::quiet_NaN();
    }

    // yyyy-mm
    if (s.size() == 7 && s[4] == '-')
        return dateToDoubleAndColumnTypeHandling(QDateTime::fromString(viewToQString(s), "yyyy-MM"), ChartDataType::dateOnlyYearMonth);

    // [d]d.[m]m.yyyy
    std::regex regex(R"((?:^|^\w\w )(\d{1,2})(?:\.)(\d{1,2})(?:\.)(\d\d\d\d)$)");
    std::cmatch baseMatch;
    if (std::regex_match(svUtf8.beginRaw(), baseMatch, regex) && baseMatch.size() == 4)
    {
        const auto asInt = [](const std::csub_match& subMatch) { return ::DFG_MODULE_NS(str)::strTo<int>(StringViewC(subMatch.first, subMatch.second)); };
        // 0 has entire match, so actual captures start from index 1.
        return dateToDoubleAndColumnTypeHandling(QDateTime(QDate(asInt(baseMatch[3]), asInt(baseMatch[2]), asInt(baseMatch[1]))), ChartDataType::dateOnly);

    }

    if (s.size() >= 8 && s[2] == ':' && s[5] == ':')
    {
        if (s.size() >= 10 && s[8] == '.')
        {
            updateColumnDataType(ChartDataType::dayTimeMillisecond);
            return timeToDouble(QTime::fromString(viewToQString(s), "hh:mm:ss.zzz"));
        }
        else
        {
            updateColumnDataType(ChartDataType::dayTime);
            return timeToDouble(QTime::fromString(viewToQString(s), "hh:mm:ss"));
        }
    }
    if (!::DFG_MODULE_NS(alg)::contains(s, ','))
        return stringToDouble(svUtf8.asUntypedView()); // Not using s directly because at the time of writing it caused redundant string-object to be created due to shortcomings in strTo().
    else
    {
        auto s2 = viewToQString(s);
        s2.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
        return stringToDouble(s2);
    }
}

auto ::DFG_MODULE_NS(qt)::GraphDataSource::columnDataType(const DataSourceIndex nCol) const -> ChartDataType
{
    const auto colTypes = columnDataTypes();
    auto iter = colTypes.find(nCol);
    return (iter != colTypes.end()) ? iter->second : ChartDataType();
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
            rv.logLevel(gConsoleLogHandle.effectiveLevel());
        }
    }
    else // Case: entry does not have log_level field. Setting level to global default.
        rv.logLevel(gConsoleLogHandle.effectiveLevel());

    // Reading operations
    {
        auto& manager = operationManager();
        rv.forEachPropertyId([&](const StringViewUtf8& svKey)
        {
            if (!::DFG_MODULE_NS(str)::beginsWith(svKey, StringViewUtf8(SzPtrUtf8(ChartObjectFieldIdStr_operation))))
                return;
            const auto sOperationDef = rv.fieldValueStr(svKey.asUntypedView());
            auto op = manager.createOperation(sOperationDef);
            if (!op)
            {
                if (rv.isLoggingAllowedForLevel(LogLevel::warning))
                    rv.log(LogLevel::warning, tr("Unable to create '%1: %2'").arg(viewToQString(svKey)).arg(viewToQString(sOperationDef)));
            }
            else
            {
                StringViewUtf8 svOperationOrderTag(SzPtrUtf8(svKey.beginRaw() + ::DFG_MODULE_NS(str)::strLen(ChartObjectFieldIdStr_operation)), SzPtrUtf8(svKey.endRaw()));
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
                this->log(LogLevel::warning, tr("operation '%1' encountered errors, %2").arg(viewToQString(opCopy.m_sDefinition), formatOperationErrorsForUserVisibleText(opCopy)));
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
    source.forEachElement_byColumn(nColumn, queryDetails, [&](const SourceDataSpan& sourceData)
    {
        inserter(destValues, sourceData);
    });
    destValues.setSorting(true);
    m_columnTypes = source.columnDataTypes();
    m_columnNames = source.columnNames();
    this->m_bIsValid = true;
    return true;
}


bool DFG_MODULE_NS(qt)::TableSelectionCacheItem::storeColumnFromSource(GraphDataSource& source, const DataSourceIndex nColumn)
{
    const auto inserter = [&](RowToValueMap& values, const SourceDataSpan& sourceData)
    {
        values.pushBackToUnsorted(sourceData.rows(), sourceData.doubles());
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
    // As there's no mechanism to verify if cacheItem is to be used by someone else, always copying.
    return (iter != m_colToValuesMap.end()) ? iter->second : RowToValueMap();
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
    TableSelectionOptional getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry, std::array<DataSourceIndex, 2>& rColumnIndexes, std::array<bool, 2>& rRowFlags);

    CacheKeyToTableSelectionaMap m_tableSelectionDatas;
}; // ChartDataCache


auto DFG_MODULE_NS(qt)::ChartDataCache::getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry, std::array<DataSourceIndex, 2>& rColumnIndexes, std::array<bool, 2>& rRowFlags) -> TableSelectionOptional
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

    auto& rCacheItem = *iter->second;

    const auto nLastColumnIndex = columns.back();
    const auto nDefaultValue = nLastColumnIndex + 1;

    auto& xColumnIndex = rColumnIndexes[0];
    auto& yColumnIndex = rColumnIndexes[1];
    xColumnIndex = getChosenColumnIndex(source, ChartObjectFieldIdStr_xSource, defEntry, nDefaultValue);
    yColumnIndex = getChosenColumnIndex(source, ChartObjectFieldIdStr_ySource, defEntry, nDefaultValue);

    const auto isRowIndexSpecifier = [=](const DataSourceIndex i) { return i == nLastColumnIndex + 2; };

    if (xColumnIndex == GraphDataSource::invalidIndex() || yColumnIndex == GraphDataSource::invalidIndex())
        return TableSelectionOptional(); // Either column was defined but not found.

    auto& bXisRowIndex = rRowFlags[0];
    auto& bYisRowIndex = rRowFlags[1];
    bXisRowIndex = isRowIndexSpecifier(xColumnIndex);
    bYisRowIndex = isRowIndexSpecifier(yColumnIndex);

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

    makeEffectiveColumnIndexes(xColumnIndex, yColumnIndex, columns, isRowIndexSpecifier);

    const auto bStringsNeededForX = (!bXisRowIndex && (defEntry.isType(ChartObjectChartTypeStr_bars) || (defEntry.isType(ChartObjectChartTypeStr_histogram) && defEntry.fieldValueStr(ChartObjectFieldIdStr_binType) == DFG_UTF8("text"))));
    const auto bStringsNeededForY = (!bYisRowIndex && defEntry.isType(ChartObjectChartTypeStr_histogram) && defEntry.fieldValueStr(ChartObjectFieldIdStr_binType) == DFG_UTF8("text"));

    const auto bXsuccess = (bStringsNeededForX) ? rCacheItem.storeColumnFromSource_strings(source, xColumnIndex) : rCacheItem.storeColumnFromSource(source, xColumnIndex);
    bool bYsuccess = false;
    if (bXsuccess)
        bYsuccess = (bStringsNeededForY) ? rCacheItem.storeColumnFromSource_strings(source, yColumnIndex) : rCacheItem.storeColumnFromSource(source, yColumnIndex);

    if (bXsuccess && bYsuccess)
        return iter->second;
    else
        return TableSelectionOptional(); // Failed to read columns
}

void DFG_MODULE_NS(qt)::ChartDataCache::removeInvalidCaches()
{
    ::DFG_MODULE_NS(cont)::eraseIf(m_tableSelectionDatas, [](const decltype(*m_tableSelectionDatas.begin())& pairItem)
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

    void showGuideWidget();

    static QString getGuideString();

    ChartDefinition getChartDefinition();

    ChartDefinitionViewer getChartDefinitionViewer();

    ChartController* getController();

    void onTextDefinitionChanged();

    void updateChartDefinitionViewable();
    void checkUpdateChartDefinitionViewableTimer();

    QObjectStorage<JsonListWidget> m_spRawTextDefinition; // Guaranteed to be non-null between constructor and destructor.
    QObjectStorage<QWidget> m_spGuideWidget;
    QPointer<GraphControlPanel> m_spParent;
    ChartDefinitionViewable m_chartDefinitionViewable;
    QElapsedTimer m_timeSinceLastEdit;
    QTimer m_chartDefinitionTimer;
    const int m_nChartDefinitionUpdateMinimumIntervalInMs = 250;
}; // Class GraphDefinitionWidget


GraphDefinitionWidget::GraphDefinitionWidget(GraphControlPanel *pNonNullParent) :
    BaseClass(pNonNullParent),
    m_spParent(pNonNullParent)
{
    DFG_ASSERT(m_spParent.data() != nullptr);

    auto spLayout = std::make_unique<QVBoxLayout>(this);

    m_spRawTextDefinition.reset(new JsonListWidget(this));
    m_spRawTextDefinition->setPlainText(GraphDefinitionEntry::xyGraph(QString(), QString()).toText());
    DFG_QT_VERIFY_CONNECT(connect(m_spRawTextDefinition.get(), &JsonListWidget::textChanged, this, &GraphDefinitionWidget::onTextDefinitionChanged));

    spLayout->addWidget(m_spRawTextDefinition.get());

    // Adding control buttons
    {
        auto spHlayout = std::make_unique<QHBoxLayout>();
        auto pApplyButton = new QPushButton(tr("Apply"), this); // Deletion through parentship.
        auto pGuideButton = new QPushButton(tr("Show guide"), this); // Deletion through parentship.

        auto pController = getController();
        if (pController)
            DFG_QT_VERIFY_CONNECT(connect(pApplyButton, &QPushButton::clicked, pController, &ChartController::refresh));
        DFG_QT_VERIFY_CONNECT(connect(pGuideButton, &QPushButton::clicked, this, &GraphDefinitionWidget::showGuideWidget));

        spHlayout->addWidget(pApplyButton);
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

QString GraphDefinitionWidget::getGuideString()
{
    // While waiting for std::embed (http://open-std.org/JTC1/SC22/WG21/docs/papers/2020/p1040r6.html),
    // importing the actual guide from another file with #include.
    // Note that while this is in Qt code, using qrc resources seems tricky as part of library there's no executable
    // or compiled library to embed to without introducing additional build requirements.
    return QString(
#include "res/chartGuide.html"
    );
}

void GraphDefinitionWidget::showGuideWidget()
{
    if (!m_spGuideWidget)
    {
        m_spGuideWidget.reset(new QDialog(this));
        m_spGuideWidget->setWindowTitle(tr("Chart guide"));
        auto pTextEdit = new QTextEdit(m_spGuideWidget.get()); // Deletion through parentship.
        auto pLayout = new QHBoxLayout(m_spGuideWidget.get());
        pTextEdit->setTextInteractionFlags(Qt::TextBrowserInteraction);
        pTextEdit->setHtml(getGuideString());
        
        // At least on Qt 5.13 with MSVC2017 default font size was a bit small (8.25), increasing at least to 11.
        {
            auto font = pTextEdit->currentFont();
            const auto oldPointSizeF = font.pointSizeF();
            font.setPointSizeF(Max(qreal(11), oldPointSizeF));
            pTextEdit->setFont(font);
        }

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

// Implementations for QCustomPlot
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// DFG_ALLOW_QCUSTOMPLOT  --------------------->>>>
//
//////////////////////////////////////////////////////////////////////////////////////////////////////


template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, ValueCont_T&& yVals);

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals);

static void fillQcpPlottable(QCPAbstractPlottable* pPlottable, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData);

// Defines custom implementation for ChartObject. This is used to avoid repeating virtual overrides in ChartObjects.
class ChartObjectQCustomPlot : public ::DFG_MODULE_NS(charts)::ChartObject
{
public:
    ChartObjectQCustomPlot(QCPAbstractPlottable* pPlottable)
        : m_spPlottable(pPlottable)
    {}
    virtual ~ChartObjectQCustomPlot() override {}

private:
    void setNameImpl(const ChartObjectStringView s) override
    {
        if (m_spPlottable)
            m_spPlottable->setName(viewToQString(s));
    }

    void setLineColourImpl(ChartObjectStringView svLineColour) override;

    void setFillColourImpl(ChartObjectStringView svFillColour) override;

    void setColourImpl(ChartObjectStringView svColour, std::function<void(QCPAbstractPlottable&, const QColor&)> setter);

    QPointer<QCPAbstractPlottable> m_spPlottable;
}; // class ChartObjectQCustomPlot

void ChartObjectQCustomPlot::setColourImpl(ChartObjectStringView svColour, std::function<void(QCPAbstractPlottable&, const QColor&)> setter)
{
    if (!m_spPlottable || svColour.empty())
        return;
    const QString s = viewToQString(svColour);
    QColor color(s);
    if (color.isValid())
        setter(*m_spPlottable, color);
    else
        DFG_QT_CHART_CONSOLE_WARNING(m_spPlottable->tr("Unable to parse colour with definition %1").arg(s));
}

void ChartObjectQCustomPlot::setLineColourImpl(ChartObjectStringView svLineColour)
{
    setColourImpl(svLineColour, [](QCPAbstractPlottable& plottable, const QColor& color)
    {
        auto pen = plottable.pen();
        pen.setColor(color);
        plottable.setPen(pen);
    });
}

void ChartObjectQCustomPlot::setFillColourImpl(ChartObjectStringView svFillColour)
{
    setColourImpl(svFillColour, [](QCPAbstractPlottable& plottable, const QColor& color)
    {
        plottable.setBrush(color);
    });
}

class XySeriesQCustomPlot : public ::DFG_MODULE_NS(charts)::XySeries
{
private:
    XySeriesQCustomPlot(QCPAbstractPlottable* pQcpObject);
public:
    XySeriesQCustomPlot(QCPGraph* pQcpGraph) : XySeriesQCustomPlot(static_cast<QCPAbstractPlottable*>(pQcpGraph)) {}
    XySeriesQCustomPlot(QCPCurve* pQcpCurve) : XySeriesQCustomPlot(static_cast<QCPAbstractPlottable*>(pQcpCurve)) {}

    template <class QCPObject_T, class Constructor_T>
    bool resizeImpl(QCPObject_T* pQcpObject, const DataSourceIndex nNewSize, Constructor_T constructor)
    {
        if (!pQcpObject)
            return false;
        auto spData = pQcpObject->data();
        if (!spData)
            return true;
        const auto nOldSize = static_cast<DataSourceIndex>(spData->size());
        if (nOldSize == nNewSize)
            return true;
        if (nNewSize < nOldSize)
        {
            if (nNewSize <= 0)
                spData->clear();
            else
                spData->removeAfter((spData->at(static_cast<int>(nNewSize - 1)))->sortKey());
        }
        else
        {
            const auto nAddCount = nNewSize - nOldSize;
            for (DataSourceIndex i = 0; i < nAddCount; ++i)
                spData->add(constructor(i, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()));

        }
        return true;
    }

    void resize(const DataSourceIndex nNewSize) override
    {
        if (!resizeImpl(getGraph(), nNewSize, [](Dummy, double x, double y) { return QCPGraphData(x, y); }))
            resizeImpl(getCurve(), nNewSize, [](DataSourceIndex i, double x, double y) { return QCPCurveData(static_cast<double>(i), x, y); });
    }

    QCPGraph* getGraph()
    {
        return qobject_cast<QCPGraph*>(m_spQcpObject.data());
    }

    QCPCurve* getCurve()
    {
        return qobject_cast<QCPCurve*>(m_spQcpObject.data());
    }

    template <class DataType_T, class QcpObject_T, class DataContructor_T>
    bool setValuesImpl(QcpObject_T* pQcpObject, InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags, DataContructor_T constructor)
    {
        if (!pQcpObject)
            return false;

        if (xVals.size() != yVals.size())
            return true;

        auto iterY = yVals.cbegin();
        QVector<DataType_T> data;
        if (pFilterFlags && pFilterFlags->size() == xVals.size())
        {
            auto iterFlag = pFilterFlags->begin();
            size_t n = 0;
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++iterFlag, ++n)
            {
                if (*iterFlag)
                    data.push_back(constructor(n, *iterX, *iterY));
            }
        }
        else
        {
            data.reserve(saturateCast<int>(xVals.size()));
            size_t n = 0;
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++n)
            {
                data.push_back(constructor(n, *iterX, *iterY));
            }
        }
        fillQcpPlottable(*pQcpObject, std::move(data));
        return true;
    }

    void setValues(InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags) override
    {
        if (!setValuesImpl<QCPGraphData>(getGraph(), xVals, yVals, pFilterFlags, [](Dummy, double x, double y) { return QCPGraphData(x,y); } ))
            setValuesImpl<QCPCurveData>(getCurve(), xVals, yVals, pFilterFlags, [](size_t n, double x, double y) { return QCPCurveData(static_cast<double>(n), x, y); });
    }

    void setLineStyle(StringViewC svStyle) override;

    void setPointStyle(StringViewC svStyle) override;

private:
    void setScatterStyle(const QCPScatterStyle style);
    // Unlike scatter style, graph and curve have distinct set of line styles, so handling them separately.
    bool setLineStyle(QCPGraph* pGraph, StringViewC svStyle); // Returns true iff non-null pGraph given.
    bool setLineStyle(QCPCurve* pCurve, StringViewC svStyle); // Returns true iff non-null pCurve given.

public:

    QPointer<QCPAbstractPlottable> m_spQcpObject;
}; // Class XySeriesQCustomPlot

XySeriesQCustomPlot::XySeriesQCustomPlot(QCPAbstractPlottable* pQcpObject)
    : m_spQcpObject(pQcpObject)
{
    setBaseImplementation<ChartObjectQCustomPlot>(m_spQcpObject.data());
}

bool XySeriesQCustomPlot::setLineStyle(QCPGraph* pGraph, StringViewC svStyle)
{
    if (!pGraph)
        return false;

    QCPGraph::LineStyle style = QCPGraph::lsNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPGraph::lsLine;
    else if (svStyle == "step_left")
        style = QCPGraph::lsStepLeft;
    else if (svStyle == "step_right")
        style = QCPGraph::lsStepRight;
    else if (svStyle == "step_middle")
        style = QCPGraph::lsStepCenter;
    else if (svStyle == "pole")
        style = QCPGraph::lsImpulse;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown line style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));
    }
    pGraph->setLineStyle(style);
    return true;
}

bool XySeriesQCustomPlot::setLineStyle(QCPCurve* pCurve, StringViewC svStyle)
{
    if (!pCurve)
        return false;

    QCPCurve::LineStyle style = QCPCurve::lsNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPCurve::lsLine;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown line style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));
    }
    pCurve->setLineStyle(style);
    return true;
}

void XySeriesQCustomPlot::setLineStyle(StringViewC svStyle)
{
    if (!setLineStyle(getGraph(), svStyle))
        setLineStyle(getCurve(), svStyle);
}

void XySeriesQCustomPlot::setScatterStyle(const QCPScatterStyle style)
{
    auto pGraph = getGraph();
    if (pGraph)
        pGraph->setScatterStyle(style);
    else
    {
        auto pCurve = getCurve();
        if (pCurve)
            pCurve->setScatterStyle(style);
    }
}

void XySeriesQCustomPlot::setPointStyle(StringViewC svStyle)
{
    if (!m_spQcpObject)
        return;
    QCPScatterStyle style = QCPScatterStyle::ssNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPScatterStyle::ssCircle;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown point style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));
        
    }
    setScatterStyle(style);
}


class HistogramQCustomPlot : public ::DFG_MODULE_NS(charts)::Histogram
{
public:
    HistogramQCustomPlot(QCPBars* pBars);
    ~HistogramQCustomPlot() override;

    void setValues(InputSpanD xVals, InputSpanD yVals) override;

    // Returns effective bar width which is successful case is the same as given argument.
    double setBarWidth(double width);

    QPointer<QCPBars> m_spBars; // QCPBars is owned by QCustomPlot, not by *this.
}; // Class HistogramQCustomPlot

HistogramQCustomPlot::HistogramQCustomPlot(QCPBars* pBars)
{
    m_spBars = pBars;
    if (!m_spBars)
        return;
    setBaseImplementation<ChartObjectQCustomPlot>(m_spBars.data());
}

HistogramQCustomPlot::~HistogramQCustomPlot()
{
}

void HistogramQCustomPlot::setValues(InputSpanD xVals, InputSpanD yVals)
{
    if (!m_spBars)
        return;
    fillQcpPlottable<QCPBarsData>(*m_spBars, xVals, yVals);

    m_spBars->rescaleAxes(); // TODO: revise, probably not desirable when there are more than one plottable in canvas.
}

double HistogramQCustomPlot::setBarWidth(const double width)
{
    if (!m_spBars)
        return std::numeric_limits<double>::quiet_NaN();
    m_spBars->setWidthType(QCPBars::wtPlotCoords);
    m_spBars->setWidth(width);
    return m_spBars->width();
}

class BarSeriesQCustomPlot : public ::DFG_MODULE_NS(charts)::BarSeries
{
public:
    BarSeriesQCustomPlot(QCPBars* pBars);
    ~BarSeriesQCustomPlot() override;

    QPointer<QCPBars> m_spBars; // QCPBars is owned by QCustomPlot, not by *this.
}; // Class HistogramQCustomPlot

BarSeriesQCustomPlot::BarSeriesQCustomPlot(QCPBars* pBars)
{
    m_spBars = pBars;
    if (!m_spBars)
        return;
    setBaseImplementation<ChartObjectQCustomPlot>(m_spBars.data());
}

BarSeriesQCustomPlot::~BarSeriesQCustomPlot()
{
}

class ChartPanel;

using PointXy = ::DFG_MODULE_NS(cont)::TrivialPair<double, double>;

class ToolTipTextStream
{
public:
    ToolTipTextStream& operator<<(const QString& str)
    {
        m_sText += str;
        return *this;
    }

    const QString& toPlainText() const
    {
        return m_sText;
    }

    QString numberToText(const double d) const
    {
        return QString::number(d);
    }

    QString numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker) const
    {
        if (!pTimeTicker || ::DFG_MODULE_NS(math)::isNan(d))
            return numberToText(d);
        else
        {
            const auto timeSpec = pTimeTicker->dateTimeSpec();
            auto dt = pTimeTicker->keyToDateTime(d).toTimeSpec(timeSpec);
            return dt.toString(pTimeTicker->dateTimeFormat());
        }
    }

    QString numberToText(const double d, QCPAxisTickerText* pTextTicker) const // Note: pTextTicker param is not const because QCPAxisTickerText doesn't seem to provide const-way of asking ticks.
    {
        return (!pTextTicker) ? numberToText(d) : pTextTicker->ticks().value(d);
    }

    bool isDestinationEmpty() const { return m_sText.isEmpty(); }

    QString m_sText;
}; // class ToolTipTextStream

class ChartCanvasQCustomPlot : public DFG_MODULE_NS(charts)::ChartCanvas, public QObject
{
public:
    using ChartEntryOperation       = ::DFG_MODULE_NS(charts)::ChartEntryOperation;
    using ChartEntryOperationList   = ::DFG_MODULE_NS(charts)::ChartEntryOperationList;

    ChartCanvasQCustomPlot(QWidget* pParent = nullptr);

          QCustomPlot* getWidget()       { return m_spChartView.get(); }
    const QCustomPlot* getWidget() const { return m_spChartView.get(); }

    void setTitle(StringViewUtf8 svPanelId, StringViewUtf8 svTitle) override;

    bool hasChartObjects() const override;

    void addXySeries() override;

    void optimizeAllAxesRanges() override;

    void addContextMenuEntriesForChartObjects(void* pMenu) override;

    void removeAllChartObjects() override;

    void repaintCanvas() override;

    ChartObjectHolder<XySeries>  createXySeries(const XySeriesCreationParam& param)   override;
    ChartObjectHolder<Histogram> createHistogram(const HistogramCreationParam& param) override;
    ChartObjectHolder<BarSeries> createBarSeries(const BarSeriesCreationParam& param) override;

    void setAxisLabel(StringViewUtf8 sPanelId, StringViewUtf8 axisId, StringViewUtf8 axisLabel) override;
    void setAxisTickLabelDirection(StringViewUtf8 sPanelId, StringViewUtf8 axisId, StringViewUtf8 value) override;

    bool isLegendSupported() const override { return true; }
    bool isToolTipSupported() const override { return true; }
    bool isLegendEnabled() const override;
    bool isToolTipEnabled() const override { return m_bToolTipEnabled; }
    bool enableLegend(bool) override;
    bool enableToolTip(bool) override;
    void createLegends() override;

    // Implementation details
    QCPAxisRect* getAxisRect(const ChartObjectCreationParam& param);
    QCPAxis* getAxis(const ChartObjectCreationParam& param, QCPAxis::AxisType axisType);
    QCPAxis* getXAxis(const ChartObjectCreationParam& param);
    QCPAxis* getYAxis(const ChartObjectCreationParam& param);
    QCPAxisRect* getAxisRect(const StringViewUtf8& svPanelId);
    QCPAxis* getAxis(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId);
    bool getGridPos(const StringViewUtf8 svPanelId, int& nRow, int& nCol);

    // Returns ChartPanel on which given (mouse) cursor is.
    ChartPanel* getChartPanel(const QPoint& pos); // pos is as available in mouseMoveEvent() pEvent->pos()

    void removeLegends();

    void mouseMoveEvent(QMouseEvent* pEvent);

    void applyChartOperationsTo(QPointer<QCPAbstractPlottable> spPlottable);
    void applyChartOperationTo(QPointer<QCPAbstractPlottable> spPlottable, const StringViewUtf8& svOperationId);

    // Returns true if got non-null object as argument.
    static bool toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPCurve* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPBars* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);

private:
    template <class This_T, class Func_T>
    static void forEachAxisRectImpl(This_T& rThis, Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func) const;

    template <class Func_T> static void forEachAxis(QCPAxisRect* pAxisRect, Func_T&& func);

    // Returns true if operations were successfully created from given definition list, false otherwise.
    bool applyChartOperationsTo(QCPAbstractPlottable* pPlottable, const QStringList& definitionList);
    void applyChartOperationsTo(QCPAbstractPlottable* pPlottable, ChartEntryOperationList& operations);
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData createOperationPipeData(QCPAbstractPlottable* pPlottable);

public:

    QObjectStorage<QCustomPlot> m_spChartView;
    bool m_bLegendEnabled = false;
    bool m_bToolTipEnabled = true;
    QVector<QCPLegend*> m_legends; // All but the default legend
}; // ChartCanvasQCustomPlot


class ChartPanel : public QCPLayoutGrid
{
    //Q_OBJECT
public:
    using PairT = PointXy;
    using AxisT = QCPAxis;
    ChartPanel(QCustomPlot* pQcp, StringViewUtf8 svPanelId);
    QCPAxisRect* axisRect();

    void setTitle(StringViewUtf8 svTitle);
    QString getTitle() const;

    QString getPanelId() const { return m_panelId; }

    // Returns (x, y) pair from pixelToCoord() of primary axes.
    PairT pixelToCoord_primaryAxis(const QPoint& pos);

    void forEachChartObject(std::function<void(const QCPAbstractPlottable&)> handler);

    AxisT* primaryXaxis();
    AxisT* primaryYaxis();
    AxisT* secondaryXaxis();
    AxisT* secondaryYaxis();

    AxisT* axis(AxisT::AxisType axisType);

    QCustomPlot* m_pQcp = nullptr;
    QString m_panelId;
}; // class ChartPanel

ChartPanel::ChartPanel(QCustomPlot* pQcp, StringViewUtf8 svPanelId)
    : m_pQcp(pQcp)
    , m_panelId(QString::fromUtf8(svPanelId.beginRaw(), svPanelId.sizeAsInt()))
{
}

auto ChartPanel::axisRect() -> QCPAxisRect*
{
    const auto nElemCount = this->elementCount();
    if (nElemCount == 0 || (nElemCount == 1 && qobject_cast<QCPTextElement*>(this->elementAt(0)) != nullptr))
        addElement(nElemCount, 0, new QCPAxisRect(m_pQcp));
    return qobject_cast<QCPAxisRect*>(elementAt(elementCount() - 1));
}

auto ChartPanel::axis(AxisT::AxisType axisType) -> AxisT*
{
    auto pAxisRect = axisRect();
    return (pAxisRect) ? pAxisRect->axis(axisType) : nullptr;
}

auto ChartPanel::primaryXaxis() -> AxisT*
{
    return axis(AxisT::atBottom);
}

auto ChartPanel::primaryYaxis() -> AxisT*
{
    return axis(AxisT::atLeft);
}

auto ChartPanel::secondaryXaxis() -> AxisT*
{
    return nullptr;
}

auto ChartPanel::secondaryYaxis() -> AxisT*
{
    return nullptr;
}

QString ChartPanel::getTitle() const
{
    auto pTitle = qobject_cast<QCPTextElement*>(element(0, 0));
    return (pTitle) ? pTitle->text() : QString();
}

void ChartPanel::setTitle(StringViewUtf8 svTitle)
{
    auto pTitle = qobject_cast<QCPTextElement*>(element(0, 0));
    if (svTitle.empty())
    {
        // In case of empty title removing the text element.
        if (pTitle)
        {
            this->remove(pTitle);
            this->simplify(); // To remove the empty space.
        }
    }
    else
    {
        if (!pTitle)
        {
            pTitle = new QCPTextElement(this->parentPlot());
            this->insertRow(0); // insert an empty row above the axis rect
            addElement(0, 0, pTitle);
        }
        pTitle->setText(viewToQString(svTitle));
        pTitle->setFont(QFont("sans", 12, QFont::Bold));
    }
}

auto ChartPanel::pixelToCoord_primaryAxis(const QPoint& pos) -> PairT
{
    auto pXaxis = primaryXaxis();
    auto pYaxis = primaryYaxis();
    const auto x = (pXaxis) ? pXaxis->pixelToCoord(pos.x()) : std::numeric_limits<double>::quiet_NaN();
    const auto y = (pYaxis) ? pYaxis->pixelToCoord(pos.y()) : std::numeric_limits<double>::quiet_NaN();
    return PairT(x, y);
}

void ChartPanel::forEachChartObject(std::function<void(const QCPAbstractPlottable&)> handler)
{
    if (!m_pQcp || !handler)
        return;
    const auto nCount = m_pQcp->plottableCount();
    for (int i = 0; i < nCount; ++i)
    {
        auto pPlottable = m_pQcp->plottable(i);
        if (pPlottable && pPlottable->keyAxis() == this->primaryXaxis())
            handler(*pPlottable);
    }
}

namespace
{
    template <class Func_T>
    void forEachQCustomPlotLineStyle(const QCPGraph*, Func_T&& func)
    {
        func(QCPGraph::lsNone,       QT_TR_NOOP("None"));
        func(QCPGraph::lsLine,       QT_TR_NOOP("Line"));
        func(QCPGraph::lsStepLeft,   QT_TR_NOOP("Step, left-valued"));
        func(QCPGraph::lsStepRight,  QT_TR_NOOP("Step, right-valued"));
        func(QCPGraph::lsStepCenter, QT_TR_NOOP("Step middle"));
        func(QCPGraph::lsImpulse,    QT_TR_NOOP("Impulse"));
    }

    template <class Func_T>
    void forEachQCustomPlotLineStyle(const QCPCurve*, Func_T&& func)
    {
        func(QCPCurve::lsNone, QT_TR_NOOP("None"));
        func(QCPCurve::lsLine, QT_TR_NOOP("Line"));
    }

    template <class Func_T>
    void forEachQCustomPlotScatterStyle(Func_T&& func)
    {
        func(QCPScatterStyle::ssNone,              QT_TR_NOOP("None"));
        func(QCPScatterStyle::ssDot,               QT_TR_NOOP("Single pixel"));
        func(QCPScatterStyle::ssCross,             QT_TR_NOOP("Cross"));
        func(QCPScatterStyle::ssPlus,              QT_TR_NOOP("Plus"));
        func(QCPScatterStyle::ssCircle,            QT_TR_NOOP("Circle"));
        func(QCPScatterStyle::ssDisc,              QT_TR_NOOP("Disc"));
        func(QCPScatterStyle::ssSquare,            QT_TR_NOOP("Square"));
        func(QCPScatterStyle::ssDiamond,           QT_TR_NOOP("Diamond"));
        func(QCPScatterStyle::ssStar,              QT_TR_NOOP("Star"));
        func(QCPScatterStyle::ssTriangle,          QT_TR_NOOP("Triangle"));
        func(QCPScatterStyle::ssTriangleInverted,  QT_TR_NOOP("Inverted triangle"));
        func(QCPScatterStyle::ssCrossSquare,       QT_TR_NOOP("CrossSquare"));
        func(QCPScatterStyle::ssPlusSquare,        QT_TR_NOOP("PlusSquare"));
        func(QCPScatterStyle::ssCrossCircle,       QT_TR_NOOP("CrossCircle"));
        func(QCPScatterStyle::ssPlusCircle,        QT_TR_NOOP("PlusCircle"));
        func(QCPScatterStyle::ssPeace,             QT_TR_NOOP("Peace"));
    }
}

namespace
{
    template <class QCPObject_T, class Style_T, class StyleSetter_T>
    static void addGraphStyleAction(QMenu& rMenu, QCPObject_T& rQcpObject, QCustomPlot& rCustomPlot, const Style_T currentStyle, const Style_T style, const QString& sStyleName, StyleSetter_T styleSetter)
    {
        auto pAction = rMenu.addAction(sStyleName, [=, &rQcpObject, &rCustomPlot]() { (rQcpObject.*styleSetter)(style); rCustomPlot.replot(); });
        if (pAction)
        {
            pAction->setCheckable(true);
            if (currentStyle == style)
                pAction->setChecked(true);
        }
    }
}

ChartCanvasQCustomPlot::ChartCanvasQCustomPlot(QWidget* pParent)
{
    m_spChartView.reset(new QCustomPlot(pParent));

    const auto interactions = m_spChartView->interactions();
    // iRangeZoom: "Axis ranges are zoomable with the mouse wheel"
    // iRangeDrag: Allows moving view point by dragging.
    // iSelectPlottables: "Plottables are selectable"
    // iSelectLegend: "Legends are selectable"
    m_spChartView->setInteractions(interactions | QCP::iRangeZoom | QCP::iRangeDrag | QCP::iSelectPlottables | QCP::iSelectLegend);

    DFG_QT_VERIFY_CONNECT(connect(m_spChartView.get(), &QCustomPlot::mouseMove, this, &ChartCanvasQCustomPlot::mouseMoveEvent));
}

bool ChartCanvasQCustomPlot::hasChartObjects() const
{
    return m_spChartView && (m_spChartView->plottableCount() > 0);
}

void ChartCanvasQCustomPlot::addXySeries()
{
    if (!m_spChartView)
        return;
    m_spChartView->addGraph();
}

namespace
{
    // Returns true iff pObj was non-null
    template<class T> 
    static bool addContextMenuEntriesForXyType(QCustomPlot& rQcp, QMenu& rMenu, T* pObj)
    {
        if (!pObj)
            return false;
        // Adding line style entries
        {
            addSectionEntryToMenu(&rMenu, rMenu.tr("Line Style"));
            const auto currentLineStyle = pObj->lineStyle();
            forEachQCustomPlotLineStyle(pObj, [&](const T::LineStyle style, const char* pszStyleName)
            {
                addGraphStyleAction(rMenu, *pObj, rQcp, currentLineStyle, style, rMenu.tr(pszStyleName), &T::setLineStyle);
            });
        }

        // Adding point style entries
        {
            addSectionEntryToMenu(&rMenu, rMenu.tr("Point Style"));
            const auto currentPointStyle = pObj->scatterStyle().shape();
            forEachQCustomPlotScatterStyle([&](const QCPScatterStyle::ScatterShape style, const char* pszStyleName)
            {
                addGraphStyleAction(rMenu, *pObj, rQcp, currentPointStyle, style, rMenu.tr(pszStyleName), &T::setScatterStyle);
            });
        }
        return true;
    }
}

void ChartCanvasQCustomPlot::addContextMenuEntriesForChartObjects(void* pMenuHandle)
{
    if (!m_spChartView || !pMenuHandle)
        return;
    auto pMenuRawObj = reinterpret_cast<QObject*>(pMenuHandle);
    auto pMenu = qobject_cast<QMenu*>(pMenuRawObj);
    if (!pMenu)
        return;

    QMenu& menu = *pMenu;

    /*
    According to class inheritance tree, plottable can be one of these:
    QCPGraph, QCPCurve, QCPBars, QCPStatisticalBox, QCPColorMap, QCPFinancial
    */
    const auto nPlottableCount = m_spChartView->plottableCount();
    for (int i = 0; i < nPlottableCount; ++i)
    {
        auto pPlottable = m_spChartView->plottable(i);
        if (!pPlottable)
            continue;
        const auto name = pPlottable->name();

        // TODO: limit length
        // TODO: icon based on object type (xy, histogram...)
        auto pSubMenu = menu.addMenu(pPlottable->name());
        if (!pSubMenu)
            continue;

        // Adding menu title
        addTitleEntryToMenu(pSubMenu, pPlottable->name());

        // Adding remove-entry
        pSubMenu->addAction(tr("Remove"), [=]() { m_spChartView->removePlottable(pPlottable); repaintCanvas(); });

        // Adding operations-menu
        {
            auto pOperationsMenu = pSubMenu->addMenu(tr("Operations"));
            if (pOperationsMenu)
            {
                QPointer<QCPAbstractPlottable> spPlottable = pPlottable;
                pOperationsMenu->addAction(tr("Apply multiple..."), [=]() { applyChartOperationsTo(spPlottable); });
                pOperationsMenu->addSeparator();
                operationManager().forEachOperationId([&](StringUtf8 sOperationId)
                {
                    // TODO: add operation only if it accepts input type that chart object provides.
                    pOperationsMenu->addAction(viewToQString(sOperationId), [=]() { applyChartOperationTo(spPlottable, sOperationId); });
                });
                
            }
        }

        if (addContextMenuEntriesForXyType(*m_spChartView, *pSubMenu, qobject_cast<QCPGraph*>(pPlottable)))
            continue;
        if (addContextMenuEntriesForXyType(*m_spChartView, *pSubMenu, qobject_cast<QCPCurve*>(pPlottable)))
            continue;

        auto pBars = qobject_cast<QCPBars*>(pPlottable);
        if (pBars)
        {
            addSectionEntryToMenu(pSubMenu, tr("Bar controls not implemented"));
            continue;
        }
    }
}

void ChartCanvasQCustomPlot::removeAllChartObjects()
{
    auto p = getWidget();
    if (!p)
        return;
    p->clearPlottables();

    // Removing legends
    removeLegends();

    // Removing all but first panel
    auto pPlotLayout = p->plotLayout();
    if (pPlotLayout)
    {
        while (pPlotLayout->elementCount() > 0 && pPlotLayout->removeAt(pPlotLayout->elementCount() - 1))
        {
        }
        auto pFirstPanel = (pPlotLayout->elementCount() >= 1) ? dynamic_cast<ChartPanel*>(pPlotLayout->elementAt(0)) : nullptr;
        if (pFirstPanel)
            pFirstPanel->setTitle(StringViewUtf8());
        pPlotLayout->simplify(); // This removes the empty space that removed items free.

        // Removing axis labels
        forEachAxisRect([&](QCPAxisRect& axisRect)
        {
            forEachAxis(&axisRect, [](QCPAxis& rAxis)
            {
                rAxis.setLabel(QString());
            });
        });
    }

    repaintCanvas();
}

namespace
{
    void createDateTimeTicker(QCPAxis& rAxis, const QLatin1String sFormat)
    {
        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setDateTimeFormat(sFormat);
        dateTicker->setDateTimeSpec(Qt::UTC);
        rAxis.setTicker(dateTicker);
    }

    void setAxisTicker(QCPAxis& rAxis, const ChartDataType type)
    {
        // TODO: should use the same date format as in input data and/or have the format customisable.
        if (type == ChartDataType::dateAndTimeMillisecondTz)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd\nhh:mm:ss.zzzZ")); // TODO: should show in the same timezone as input
        else if (type == ChartDataType::dateAndTimeMillisecond)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd\nhh:mm:ss.zzz"));
        else if (type == ChartDataType::dateAndTimeTz)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd\nhh:mm:ssZ")); // TODO: should show in the same timezone as input
        else if (type == ChartDataType::dateAndTime)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd\nhh:mm:ss"));
        else if (type == ChartDataType::dateOnly)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd"));
        else if (type == ChartDataType::dateOnlyYearMonth)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM"));
        else if (type == ChartDataType::dayTimeMillisecond)
            createDateTimeTicker(rAxis, QLatin1String("hh:mm:ss.zzz"));
        else if (type == ChartDataType::dayTime)
            createDateTimeTicker(rAxis, QLatin1String("hh:mm:ss"));
        else
            rAxis.setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker)); // Resets to default ticker. TODO: do only if needed (i.e. if current ticker is something else than plain QCPAxisTicker)
    }

    void setAutoAxisLabel(QCPAxis& rAxis, const StringViewUtf8 svDataName)
    {
        auto sNew = rAxis.label();
        if (!sNew.isEmpty())
            sNew += QLatin1String(", ");
        sNew += viewToQString(svDataName);
        rAxis.setLabel(sNew);
    }
}

auto ChartCanvasQCustomPlot::createXySeries(const XySeriesCreationParam& param) -> ChartObjectHolder<XySeries>
{
    auto p = getWidget();
    if (!p || param.nIndex < 0)
        return nullptr;

    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;
    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to obtain axis for xySeries"));
        return nullptr;
    }

    const auto type = param.definitionEntry().graphTypeStr();

    QCPGraph* pQcpGraph = nullptr;
    QCPCurve* pQcpCurve = nullptr;
    if (type == ChartObjectChartTypeStr_xy)
        pQcpGraph = p->addGraph(pXaxis, pYaxis);
    else if (type == ChartObjectChartTypeStr_txy)
        pQcpCurve = new QCPCurve(pXaxis, pYaxis); // Owned by QCustomPlot
    else
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Invalid type '%1'").arg(untypedViewToQStringAsUtf8(type.asUntypedView())));
        return nullptr;
    }

    if (!pQcpGraph && !pQcpCurve)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error: failed to create QCP-object"));
        return nullptr;
    }

    // Setting axis type
    setAxisTicker(*pXaxis, param.xType);
    setAxisTicker(*pYaxis, param.yType);

    // Setting auto axis labels if enabled
    if (param.config().value(ChartObjectFieldIdStr_autoAxisLabels, true))
    {
        setAutoAxisLabel(*pXaxis, param.m_sXname);
        setAutoAxisLabel(*pYaxis, param.m_sYname);
    }
    if (pQcpGraph)
        return ChartObjectHolder<XySeries>(new XySeriesQCustomPlot(pQcpGraph));
    else
        return ChartObjectHolder<XySeries>(new XySeriesQCustomPlot(pQcpCurve));
}

auto ChartCanvasQCustomPlot::createHistogram(const HistogramCreationParam& param) -> ChartObjectHolder<Histogram>
{   
    // If histogram is string-type (=bin per strings, values are number of identical strings), handling it separately. It is effectively a bar chart so only need to compute counts here.
    if (!param.stringValueRange.empty())
    {
        ::DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, double> counts;
        for (const auto& s : param.stringValueRange)
            counts[s]++;
        auto spSeries = createBarSeries(BarSeriesCreationParam(param.config(), param.definitionEntry(), counts.keyRange(), counts.valueRange(), param.xType, param.m_sXname, StringUtf8()));
        auto spImpl = dynamic_cast<BarSeriesQCustomPlot*>(spSeries.get());
        return std::make_shared<HistogramQCustomPlot>(spImpl->m_spBars.data());
    }

    const auto valueRange = param.valueRange;

    if (valueRange.empty())
        return nullptr;

    auto minMaxPair = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(valueRange);
    if (*minMaxPair.first > *minMaxPair.second || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.first) || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.second))
        return nullptr;

    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;

    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create histogram, no suitable target panel found'"));
        return nullptr;
    }

    auto spHistogram = std::make_shared<HistogramQCustomPlot>(new QCPBars(pXaxis, pYaxis)); // Note: QCPBars is owned by QCustomPlot-object.

    const bool bOnlySingleValue = (*minMaxPair.first == *minMaxPair.second);

    const auto nBinCount = (!bOnlySingleValue) ? param.definitionEntry().fieldValue<int>(ChartObjectFieldIdStr_binCount, 100) : -1;

    double binWidth = std::numeric_limits<double>::quiet_NaN();

    if (nBinCount >= 0)
    {
        try
        {
            // Creating histogram points using boost::histogram.
            // Adding small adjustment to upper boundary so that items identical to max value won't get excluded from histogram.
            binWidth = (*minMaxPair.second - *minMaxPair.first) / static_cast<double>(nBinCount);
            const auto edgeAdjustment = 0.001 * binWidth;
            auto hist = boost::histogram::make_histogram(boost::histogram::axis::regular<>(static_cast<unsigned int>(nBinCount), *minMaxPair.first, *minMaxPair.second + edgeAdjustment, "x"));
            std::for_each(valueRange.begin(), valueRange.end(), std::ref(hist));

            QVector<double> xVals;
            QVector<double> yVals;
            for (auto&& x : indexed(hist, boost::histogram::coverage::all))
            {
                const double xVal = x.bin().center();
                if (DFG_MODULE_NS(math)::isNan(xVal) || DFG_MODULE_NS(math)::isInf(xVal))
                    continue;
                xVals.push_back(xVal);
                yVals.push_back(*x);
            }
            spHistogram->setValues(makeRange(xVals), makeRange(yVals));
        }
        catch (const std::exception& e)
        {
            // Failed to create histogram.
            DFG_QT_CHART_CONSOLE_ERROR(tr("Failed to create histogram, exception message: '%1'").arg(e.what()));
            return nullptr;
        }
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
        const auto keyRange = countsPerValue.keyRange();
        spHistogram->setValues(keyRange, countsPerValue.valueRange());
        if (keyRange.size() >= 2)
        {
            const auto iterEnd = keyRange.cend();
            double minDiff = std::numeric_limits<double>::infinity();
            for (auto iter = keyRange.cbegin(), iterNext = keyRange.cbegin() + 1; iterNext != iterEnd; ++iter, ++iterNext)
                minDiff = std::min(minDiff, *iterNext - *iter);
            binWidth = minDiff;
        }
        else
            binWidth = 1;
    }

    // Setting bar width
    {
        const double defaultBarWidthFactor = 1;
        const auto barWidthFactorRequest = param.definitionEntry().fieldValue<double>(ChartObjectFieldIdStr_barWidthFactor, defaultBarWidthFactor);
        const auto barWidthRequest = binWidth * barWidthFactorRequest;
        const auto actualBarWidth = spHistogram->setBarWidth(barWidthRequest);
        if (actualBarWidth != barWidthRequest)
            DFG_QT_CHART_CONSOLE_WARNING(tr("Unable to use requested bar width factor %1, using bar width %2").arg(barWidthFactorRequest).arg(actualBarWidth));
    }

    // Settings ticker for x-axis.
    setAxisTicker(*pXaxis, param.xType);

    // Setting auto axis label for x-axis if enabled
    if (param.config().value(ChartObjectFieldIdStr_autoAxisLabels, true))
        setAutoAxisLabel(*pXaxis, param.m_sXname);

    pXaxis->scaleRange(1.1); // Adds margins so that boundary lines won't get clipped by axisRect

    return spHistogram;
}

auto ChartCanvasQCustomPlot::createBarSeries(const BarSeriesCreationParam& param) -> ChartObjectHolder<BarSeries>
{
    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;

    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create histogram, no suitable target panel found'"));
        return nullptr;
    }

    const auto labelRange = param.labelRange;
    auto valueRange = param.valueRange;

    if (labelRange.empty() || labelRange.size() != valueRange.size())
        return nullptr;

    auto minMaxPair = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(valueRange);
    if (*minMaxPair.first > *minMaxPair.second || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.first) || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.second))
        return nullptr;

    // Filling x-data; ticks and labels.
    QVector<double> ticks;
    QVector<QString> labels;
    labels.resize(labelRange.sizeAsInt());
    std::transform(labelRange.begin(), labelRange.begin() + labels.size(), labels.begin(), [](const StringUtf8& s) { return QString::fromUtf8(s.c_str().c_str()); });
    ticks.resize(labels.size());
    ::DFG_MODULE_NS(alg)::generateAdjacent(ticks, 1, 1);

    // Buffer that is used for y-values if bars need to be merged
    QVector<double> yAdjustedData;

    // Handling bar merging if requested
    const auto bMergeIdentical = param.definitionEntry().fieldValue(ChartObjectFieldIdStr_mergeIdenticalLabels, false);
    if (bMergeIdentical)
    {
        ::DFG_MODULE_NS(cont)::MapVectorSoA<QString, double> uniqueValues;
        uniqueValues.setSorting(false); // Want to keep original order.
        for (int i = 0, nCount = saturateCast<int>(valueRange.size()); i < nCount; ++i)
            uniqueValues[labels[i]] += valueRange[i];
        if (uniqueValues.size() != static_cast<size_t>(ticks.size())) // Does data have duplicate bar identifiers?
        {
            const auto nNewSize = saturateCast<int>(uniqueValues.size());
            ticks.resize(nNewSize);
            labels.resize(nNewSize);
            yAdjustedData.resize(nNewSize);
            int i = 0;
            for (const auto& item : uniqueValues)
            {
                labels[i] = item.first;
                yAdjustedData[i] = item.second;
                i++;
            }
            // Recalculation y-range.
            minMaxPair = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(yAdjustedData);
            valueRange = makeRange(yAdjustedData);
        }
    }

    // Setting text ticker
    {
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        textTicker->addTicks(ticks, labels);
        pXaxis->setTicker(textTicker);
    }

    //  Setting x-axis range
    pXaxis->setRange(0, saturateCast<int>(static_cast<size_t>(ticks.size()) + 1u));

    // Setting y-axis range
    pYaxis->setRange(0, *minMaxPair.second);

    // Setting auto axis labels if enabled
    if (param.config().value(ChartObjectFieldIdStr_autoAxisLabels, true))
    {
        setAutoAxisLabel(*pXaxis, param.m_sXname);
        setAutoAxisLabel(*pYaxis, param.m_sYname);
    }

    auto pBars = new QCPBars(pXaxis, pYaxis); // Note: QCPBars is owned by QCustomPlot-object.
    fillQcpPlottable<QCPBarsData>(*pBars, makeRange(ticks), valueRange);

    auto spBarsHolder = std::make_shared<BarSeriesQCustomPlot>(pBars);
    return spBarsHolder;
}

void ChartCanvasQCustomPlot::setAxisLabel(StringViewUtf8 svPanelId, StringViewUtf8 svAxisId, StringViewUtf8 svAxisLabel)
{
    auto pAxis = getAxis(svPanelId, svAxisId);
    if (pAxis)
        pAxis->setLabel(viewToQString(svAxisLabel));
}

void ChartCanvasQCustomPlot::setAxisTickLabelDirection(StringViewUtf8 svPanelId, StringViewUtf8 svAxisId, StringViewUtf8 svValue)
{
    auto pAxis = getAxis(svPanelId, svAxisId);
    if (pAxis)
    {
        bool bOk = true;
        const auto sValue = viewToQString(svValue);
        const auto val = (!sValue.isEmpty()) ? sValue.toDouble(&bOk) : 0;
        if (bOk)
            pAxis->setTickLabelRotation(val);
        else
            DFG_QT_CHART_CONSOLE_WARNING(QString("Bad tick label direction, got '%1'. Using default").arg(sValue));
    }
}

void ChartCanvasQCustomPlot::repaintCanvas()
{
    auto p = getWidget();
    if (p)
        p->replot();
}

bool ChartCanvasQCustomPlot::isLegendEnabled() const
{
    return m_bLegendEnabled;
}

bool ChartCanvasQCustomPlot::enableLegend(bool bEnable)
{
    if (m_bLegendEnabled == bEnable)
        return m_bLegendEnabled;

    m_bLegendEnabled = bEnable;

    if (m_bLegendEnabled)
        createLegends();
    else
        removeLegends();

    repaintCanvas();
    return m_bLegendEnabled;
}

bool ChartCanvasQCustomPlot::enableToolTip(const bool b)
{
    m_bToolTipEnabled = b;
    return m_bToolTipEnabled;
}

void ChartCanvasQCustomPlot::removeLegends()
{
    // Removing legends (would get deleted automatically with AxisRect's, but doing it here expclitly to keep m_legends up-to-date.)
    qDeleteAll(m_legends);
    m_legends.clear();
    auto p = getWidget();
    if (p && p->legend)
    {
        p->legend->clearItems();
        p->legend->setVisible(false);
    }
}

void ChartCanvasQCustomPlot::createLegends()
{
    // Creating legends for all AxisRects
    auto p = getWidget();
    if (!p)
        return;

    // Clearing old legends
    removeLegends();
    
    auto axisRects = p->axisRects();
    QVector<QCPAbstractPlottable*> plottables;
    for (int i = 0, nCount = p->plottableCount(); i < nCount; ++i)
        plottables.push_back(p->plottable(i));
    for (const auto& pAxisRect : axisRects)
    {
        if (!pAxisRect)
            continue;
        if (plottables.isEmpty())
            break; // All plottables added to legends, nothing left to do.

        QCPLegend* pLegend = nullptr;
        if (pAxisRect != p->axisRect() || p->legend == nullptr)
        {
            pLegend = new QCPLegend;
            m_legends.push_back(pLegend);
            auto pLayout = pAxisRect->insetLayout();
            if (!pLayout)
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error inserLayout() returned null"));
                continue;
            }
            pLayout->addElement(pLegend , Qt::AlignTop | Qt::AlignRight);
            pLegend->setLayer(QLatin1String("legend"));
        }
        else // Case: default AxisRect, uses existing legend. Would crash if creating another.
            pLegend = p->legend;

        if (!pLegend)
            continue;

        // Add all plottables in this AxisRect to legend.
        for (int i = 0; i < plottables.size();)
        {
            auto pPlottable = plottables[i];
            if (pPlottable->keyAxis() == pAxisRect->axis(QCPAxis::atBottom)) // This is probably not very future proof in case of advanced axis options.
            {
                pPlottable->addToLegend(pLegend);
                plottables.remove(i);
            }
            else
                ++i;
        }
        pLegend->setVisible(true);
    }
    if (!plottables.isEmpty())
        DFG_QT_CHART_CONSOLE_WARNING(tr("Number of items that didn't end up in any legend: %1").arg(plottables.size()));
}

bool ChartCanvasQCustomPlot::getGridPos(const StringViewUtf8 svPanelId, int& nRow, int& nCol)
{
    auto panelId = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem::fromStableView(svPanelId);
    if (!svPanelId.empty() && (panelId.key() != DFG_UTF8("grid") || panelId.valueCount() > 2))
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Failed to retrieve grid panel: invalid panel definition '%1'").arg(viewToQString(svPanelId)));
        return false;
    }
    nRow = (panelId.key().empty()) ? 1 : panelId.valueAs<int>(0);
    nCol = (panelId.key().empty()) ? 1 : panelId.valueAs<int>(1);
    if (nRow < 1 || nCol < 1 || nRow > 1000 || nCol > 1000)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Invalid panel grid index, expecting [1, 1000], got row = %1, column = %2").arg(nRow).arg(nCol));
        return false;
    }
    // Converting from user's 1-based index to internal 0-based.
    --nRow; --nCol;
    return true;
}

template <class This_T, class Func_T>
void ChartCanvasQCustomPlot::forEachAxisRectImpl(This_T& rThis, Func_T&& func)
{
    auto pQcp = rThis.getWidget();
    if (!pQcp)
        return;
    auto axisRects = pQcp->axisRects();
    for (auto pAxisRect : axisRects)
    {
        if (pAxisRect)
            func(*pAxisRect);
    }
}

template <class Func_T> void ChartCanvasQCustomPlot::forEachAxisRect(Func_T&& func)       { forEachAxisRectImpl(*this, std::forward<Func_T>(func)); }
template <class Func_T> void ChartCanvasQCustomPlot::forEachAxisRect(Func_T&& func) const { forEachAxisRectImpl(*this, std::forward<Func_T>(func)); }

template <class Func_T>
void ChartCanvasQCustomPlot::forEachAxis(QCPAxisRect* pAxisRect, Func_T&& func)
{
    if (!pAxisRect)
        return;
    auto axes = pAxisRect->axes();
    for (auto pAxis : axes)
    {
        if (pAxis)
            func(*pAxis);
    }
}

auto ChartCanvasQCustomPlot::getAxisRect(const StringViewUtf8& svPanelId) -> QCPAxisRect*
{
    int nRow = 0;
    int nCol = 0;
    if (!getGridPos(svPanelId, nRow, nCol))
        return nullptr;

    auto p = getWidget();
    if (!p)
        return nullptr;

    auto pLayout = p->plotLayout();
    if (!pLayout)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error: layout object does not exist"));
        return nullptr;
    }
    QCPAxisRect* pExistingAxisRect = nullptr;
    ChartPanel* pChartPanel = dynamic_cast<ChartPanel*>(pLayout->element(nRow, nCol));
    if (pChartPanel)
        pExistingAxisRect = pChartPanel->axisRect();
    if (!pExistingAxisRect)
    {
        if (!pChartPanel)
        {
            pChartPanel = new ChartPanel(p, svPanelId);
            pLayout->addElement(nRow, nCol, pChartPanel);
        }
        pExistingAxisRect = pChartPanel->axisRect();
    }
    if (!pExistingAxisRect)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error: no panel object"));
        return nullptr;
    }
    return pExistingAxisRect;
}

auto ChartCanvasQCustomPlot::getChartPanel(const QPoint& pos) -> ChartPanel*
{
    auto pCustomPlot = getWidget();
    auto pMainLayout = (pCustomPlot) ? pCustomPlot->plotLayout() : nullptr;
    if (!pMainLayout)
        return nullptr;

    const auto isWithinAxisRange = [](QCPAxis* pAxis, const double val)
        {
            return (pAxis) ? pAxis->range().contains(val) : false;
        };

    const auto nElemCount = pMainLayout->elementCount();
    for (int i = 0; i < nElemCount; ++i)
    {
        auto pElement = pMainLayout->elementAt(i);
        auto pPanel = dynamic_cast<ChartPanel*>(pElement);
        if (!pPanel)
            continue;
        auto pXaxis1 = pPanel->primaryXaxis();
        auto pYaxis1 = pPanel->primaryYaxis();
        if (pXaxis1 && pYaxis1)
        {
            const auto xCoord = pXaxis1->pixelToCoord(pos.x());
            const auto yCoord = pYaxis1->pixelToCoord(pos.y());
            if (isWithinAxisRange(pXaxis1, xCoord) && isWithinAxisRange(pYaxis1, yCoord))
                return pPanel;
        }
    }
    return nullptr;
}

auto ChartCanvasQCustomPlot::getAxis(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId) -> QCPAxis*
{
    auto pAxisRect = getAxisRect(svPanelId);
    if (svAxisId == DFG_UTF8("x"))
        return pAxisRect->axis(QCPAxis::atBottom);
    else if (svAxisId == DFG_UTF8("y"))
        return pAxisRect->axis(QCPAxis::atLeft);
   
    DFG_QT_CHART_CONSOLE_WARNING(tr("Didn't find axis %1 from panel %2").arg(viewToQString(svPanelId), viewToQString(svAxisId)));
    return nullptr;
}

auto ChartCanvasQCustomPlot::getAxisRect(const ChartObjectCreationParam& param) -> QCPAxisRect*
{
    return getAxisRect(param.panelId());
}

auto ChartCanvasQCustomPlot::getAxis(const ChartObjectCreationParam& param, const QCPAxis::AxisType axisType) -> QCPAxis*
{
    auto pAxisRect = getAxisRect(param);
    return (pAxisRect) ? pAxisRect->axis(axisType) : nullptr;
}

auto ChartCanvasQCustomPlot::getXAxis(const ChartObjectCreationParam& param) -> QCPAxis*
{
    return getAxis(param, QCPAxis::atBottom);
}

auto ChartCanvasQCustomPlot::getYAxis(const ChartObjectCreationParam& param) -> QCPAxis*
{
    return getAxis(param, QCPAxis::atLeft);
}

void ChartCanvasQCustomPlot::setTitle(StringViewUtf8 svPanelId, StringViewUtf8 svTitle)
{
    auto p = getWidget();
    auto pMainLayout = (p) ? p->plotLayout() : nullptr;
    if (!pMainLayout)
        return;

    int nRow = 0;
    int nCol = 0;
    if (!getGridPos(svPanelId, nRow, nCol))
        return;

    auto pElement = pMainLayout->element(nRow, nCol);
    auto pPanel = dynamic_cast<ChartPanel*>(pElement);
    if (pPanel == nullptr)
    {
        pPanel = new ChartPanel(p, svTitle);
        if (pElement)
            pPanel->addElement(pElement); // This transfers existing element to new layout.
        pMainLayout->addElement(nRow, nCol, pPanel);
    }
    if (pPanel)
        pPanel->setTitle(svTitle);
}

namespace
{
    // Wrapper to provide cbegin() et al that are missing from QCPDataContainer
    template <class Cont_T>
    class QCPContWrapper
    {
    public:
        using value_type = typename ::DFG_MODULE_NS(cont)::ElementType<Cont_T>::type;

        QCPContWrapper(Cont_T& ref) : m_r(ref) {}

        operator       Cont_T& ()       { return m_r; }
        operator const Cont_T& () const { return m_r; }

        auto begin()        -> typename Cont_T::iterator       { return m_r.begin(); }
        auto begin() const  -> typename Cont_T::const_iterator { return m_r.constBegin(); }
        auto cbegin() const -> typename Cont_T::const_iterator { return m_r.constBegin(); }

        auto end()        -> typename Cont_T::iterator         { return m_r.end(); }
        auto end() const  -> typename Cont_T::const_iterator   { return m_r.constEnd(); }
        auto cend() const -> typename Cont_T::const_iterator   { return m_r.constEnd(); }

        int size() const { return m_r.size(); }

        Cont_T& m_r;
    };

    template <class Cont_T, class PointToText_T, class Tr_T>
    static void createNearestPointToolTipList(Cont_T& cont, const PointXy& xy, ToolTipTextStream& toolTipStream, PointToText_T pointToText, Tr_T&& tr) // Note: QCPDataContainer doesn't seem to have const begin()/end() so must take cont by non-const reference.
    {
        using DataT = typename ::DFG_MODULE_NS(cont)::ElementType<Cont_T>::type;
        const auto nearestItems = ::DFG_MODULE_NS(alg)::nearestRangeInSorted(QCPContWrapper<Cont_T>(cont), xy.first, 5, [](const DataT& dp) { return dp.key; });
        if (nearestItems.empty())
            return;

        toolTipStream << tr("<br>Nearest by x-value:");
        // Adding items left of nearest
        for (const DataT& dp : nearestItems.leftRange())
        {
            toolTipStream << QString("<br>%1").arg(pointToText(dp));
        }
        // Adding nearest item in bold
        toolTipStream << QString("<br><b>%1</b>").arg(pointToText(*nearestItems.nearest()));
        // Adding items right of nearest
        for (const DataT& dp : nearestItems.rightRange())
        {
            toolTipStream << QString("<br>%1").arg(pointToText(dp));
        }
    }

} // unnamed namespace


bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& xy, ToolTipTextStream& toolTipStream)
{
    if (!pGraph)
        return false;

    toolTipStream << tr("<br>Graph size: %1").arg(pGraph->dataCount());

    const auto spData = pGraph->data();
    if (!spData)
        return true;

    const auto axisTicker = [](QCPAxis* pAxis) { return (pAxis) ? pAxis->ticker() : nullptr; };
    auto spXticker = axisTicker(pGraph->keyAxis());
    auto spYticker = axisTicker(pGraph->valueAxis());
    auto pXdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spXticker.get());
    auto pYdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spYticker.get());

    const auto pointToText = [&](const QCPGraphData& bd)
        {
            return QString("(%1, %2)").arg(toolTipStream.numberToText(bd.key, pXdateTicker), toolTipStream.numberToText(bd.value, pYdateTicker));
        };
    createNearestPointToolTipList(*spData, xy, toolTipStream, pointToText, [&](const char* psz) { return tr(psz); });

    return true;
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPCurve* pCurve, const PointXy& /*xy*/, ToolTipTextStream& toolTipStream)
{
    if (!pCurve)
        return false;

    toolTipStream << tr("<br>Tooltip is not implemented for txy types");

    return true;
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPBars* pBars, const PointXy& xy, ToolTipTextStream& toolTipStream)
{
    if (!pBars)
        return false;

    toolTipStream << tr("<br>Bin count: %1").arg(pBars->dataCount());
    toolTipStream << tr("<br>Bin width: %1").arg(toolTipStream.numberToText(pBars->width())); // Note: probably fails for some pBars->widthType()'s

    const auto spData = pBars->data();
    if (!spData)
        return true;

    const auto axisTicker = [](QCPAxis* pAxis) { return (pAxis) ? pAxis->ticker() : nullptr; };
    auto spXticker = axisTicker(pBars->keyAxis());
    auto pXdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spXticker.get());

    if (pXdateTicker)
    {
        const auto pointToText = [&](const QCPBarsData& bd)
            {
                return QString("(%1, %2)").arg(toolTipStream.numberToText(bd.key, pXdateTicker), toolTipStream.numberToText(bd.value));
            };
        createNearestPointToolTipList(*spData, xy, toolTipStream, pointToText, [&](const char* psz) { return tr(psz); });
    }
    else // case: ticker is not datetime ticker
    {
        auto pTextTicker = dynamic_cast<QCPAxisTickerText*>(spXticker.get());
        const auto pointToText = [&](const QCPBarsData& bd)
        {
            return QString("(%1, %2)").arg(toolTipStream.numberToText(bd.key, pTextTicker), toolTipStream.numberToText(bd.value));
        };
        createNearestPointToolTipList(*spData, xy, toolTipStream, pointToText, [&](const char* psz) { return tr(psz); });
    }

    return true;
}

void ChartCanvasQCustomPlot::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (!pEvent || !m_bToolTipEnabled)
        return;    

    const auto cursorPos = pEvent->pos();

    auto pPanel = getChartPanel(cursorPos);

    if (!pPanel)
        return;

    const auto xy = pPanel->pixelToCoord_primaryAxis(cursorPos);

    ToolTipTextStream toolTipStream;
    const QString sPanelId = pPanel->getPanelId();
    const QString sPanelTitle = pPanel->getTitle();
    if (!sPanelId.isEmpty() || !sPanelTitle.isEmpty())
    {
        const QString sPanelTitlePart = (!sPanelTitle.isEmpty()) ? QString(" ('%1')").arg(sPanelTitle) : QString();
        
        toolTipStream << tr("Panel: %1%2").arg(pPanel->getPanelId(), sPanelTitlePart).toHtmlEscaped() + "<br>";
    }
    toolTipStream << QString("x = %1, y = %2").arg(toolTipStream.numberToText(xy.first), toolTipStream.numberToText(xy.second));

    // For each chart object in this panel
    pPanel->forEachChartObject([&](const QCPAbstractPlottable& plottable)
    {
        toolTipStream << QLatin1String("<br>---------------------------");
        // Name
        toolTipStream << QString("<br>'%1'").arg(plottable.name().toHtmlEscaped());
        QString sChartObjectDetails;
        if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPGraph*>(&plottable), xy, toolTipStream)) {}
        else if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPCurve*>(&plottable), xy, toolTipStream)) {}
        else if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPBars*>(&plottable), xy, toolTipStream)) {}
    });

    QToolTip::showText(pEvent->globalPos(), toolTipStream.toPlainText());
}

namespace
{
    template <class Cont_T>
    auto createOperationPipeDataImpl(const QSharedPointer<Cont_T> spData) -> ::DFG_MODULE_NS(charts)::ChartOperationPipeData
    {
        using namespace ::DFG_MODULE_NS(charts);
        if (!spData)
            return ChartOperationPipeData();

        ChartOperationPipeData pipeData;
        auto px = pipeData.editableValuesByIndex(0);
        auto py = pipeData.editableValuesByIndex(1);
        if (!px || !py)
        {
            DFG_ASSERT(false); // Not expected to ever end up here.
            return pipeData;
        }
        const auto nSize = static_cast<size_t>(spData->size());
        px->resize(nSize);
        py->resize(nSize);
        size_t i = 0;
        for (auto xy : *spData)
        {
            (*px)[i] = xy.key;
            (*py)[i] = xy.value;
            i++;
        }
        pipeData.setValueVectorsAsData();
        return pipeData;
    }
}

auto ChartCanvasQCustomPlot::createOperationPipeData(QCPAbstractPlottable* pPlottable) -> ::DFG_MODULE_NS(charts)::ChartOperationPipeData
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!pPlottable)
        return ChartOperationPipeData();
    auto pGraph = qobject_cast<QCPGraph*>(pPlottable);
    if (pGraph)
        return createOperationPipeDataImpl(pGraph->data());
    auto pCurve = qobject_cast<QCPCurve*>(pPlottable);
    if (pCurve)
        return createOperationPipeDataImpl(pCurve->data());
    auto pBars = qobject_cast<QCPBars*>(pPlottable);
    if (pBars)
        return createOperationPipeDataImpl(pBars->data());
    DFG_ASSERT_IMPLEMENTED(false);
    return ChartOperationPipeData();
}

void ChartCanvasQCustomPlot::applyChartOperationTo(QPointer<QCPAbstractPlottable> spPlottable, const StringViewUtf8& svOperationId)
{
    using namespace ::DFG_MODULE_NS(charts);
    QCPAbstractPlottable* pPlottable = spPlottable.data();
    if (!pPlottable)
        return;

    const QString sOperationId = viewToQString(svOperationId);

    QString sOperationDefinition;
    // Asking arguments from user
    {
        const QString sAdditionInfo = (qobject_cast<QCPBars*>(pPlottable) != nullptr)
            ? tr("\nNote: bar chart string labels are not passed to operation")
            : QString();
        sOperationDefinition = QInputDialog::getText(this->m_spChartView.data(),
            tr("Applying operation"),
            tr("Define operation for '%1'\nUsage: %2%3").arg(pPlottable->name(), getOperationDefinitionUsageGuide(svOperationId), sAdditionInfo),
            QLineEdit::Normal,
            getOperationDefinitionPlaceholder(svOperationId));
        if (sOperationDefinition.isEmpty())
            return;
    }
    applyChartOperationsTo(pPlottable, QStringList(sOperationDefinition));
}

void ChartCanvasQCustomPlot::applyChartOperationsTo(QPointer<QCPAbstractPlottable> spPlottable)
{
    using namespace ::DFG_MODULE_NS(charts);
    QCPAbstractPlottable* pPlottable = spPlottable.data();
    if (!pPlottable)
        return;

    // Asking operation list from user
    // TODO: showing operation guide in the UI would be nice.
    QString sInitial;
    bool bGoodOperationList = false;
    do
    {
        QStringList operationStringList;
        const QString sAdditionInfo = (qobject_cast<QCPBars*>(pPlottable) != nullptr)
            ? tr("\nNote: bar chart string labels are not passed to operations")
            : QString();
        bool bOk;
        sInitial = QInputDialog::getMultiLineText(this->m_spChartView.data(),
            tr("Applying operations"),
            tr("Define operations (one per line) for '%1'\n%2").arg(pPlottable->name()).arg(sAdditionInfo),
            sInitial,
            &bOk);
        operationStringList = sInitial.split('\n');
        if (!bOk || operationStringList.isEmpty())
            return;
        bGoodOperationList = applyChartOperationsTo(pPlottable, operationStringList);
    } while (!bGoodOperationList);
}

bool ChartCanvasQCustomPlot::applyChartOperationsTo(QCPAbstractPlottable* pPlottable, const QStringList& operationStringList)
{
    auto& manager = operationManager();
    ChartEntryOperationList operations;
    QString sErrors;
    for (const auto& sItem : operationStringList)
    {
        if (sItem.isEmpty() || sItem.front() == '#')
            continue;
        StringUtf8 sDef(SzPtrUtf8(sItem.toUtf8()));
        auto op = manager.createOperation(sDef);
        if (op)
        {
            op.m_sDefinition = std::move(sDef);
            operations.push_back(std::move(op));
        }
        else
        {
            sErrors += tr("\n'%1': ").arg(sItem);
            if (sItem.indexOf("(") != -1 && manager.hasOperation(SzPtrUtf8(sItem.mid(0, sItem.indexOf("(")).toUtf8())))
                sErrors += tr("invalid arguments");
            else
                sErrors += tr("no such operation");
        }
    }
    if (!sErrors.isEmpty())
    {
        QMessageBox::information(getWidget(), tr("Unable to create operations"), tr("Unable to create operations, the following errors were encountered:\n%1").arg(sErrors));
        return false;
    }
    applyChartOperationsTo(pPlottable, operations);
    return true;
}

void ChartCanvasQCustomPlot::applyChartOperationsTo(QCPAbstractPlottable* pPlottable, ChartEntryOperationList& operations)
{
    using namespace ::DFG_MODULE_NS(charts);

    if (!pPlottable || operations.empty())
        return;

    auto pipeData = createOperationPipeData(pPlottable);
    if (pipeData.vectorCount() == 0)
    {
        return;
    }
    operations.executeAll(pipeData);

    // Checking for errors
    QString sErrors;
    for (const auto& op : operations)
    {
        if (op.hasErrors())
        {
            sErrors += tr("\nOperation '%1': %2").arg(viewToQString(op.m_sDefinition), formatOperationErrorsForUserVisibleText(op));
        }
    }
    if (!sErrors.isEmpty())
    {
        QMessageBox::information(getWidget(),
                                 tr("Errors in operations"),
                                 tr("There were errors applying operations, changes were not applied to chart object '%1'\n\nList of errors:\n%2").arg(pPlottable->name(), sErrors));
        return;
    }

    fillQcpPlottable(pPlottable, pipeData);

    // Replotting so that the result comes visible.
    m_spChartView->replot();
}

void ChartCanvasQCustomPlot::optimizeAllAxesRanges()
{
    auto pQcp = getWidget();
    if (!pQcp)
        return;

    // Scaling doesn't seem to work correctly if plotting hasn't been made yet. So if that seems to be the case, replotting first.
    bool bDoReplotFirst = false;
    forEachAxisRect([&](QCPAxisRect& rect)
    {
        bDoReplotFirst = bDoReplotFirst || (rect.width() == 0);
    });

    if (bDoReplotFirst)
        pQcp->replot();

    pQcp->rescaleAxes();

    // Adding margin to axes so that min/max point markers won't get clipped by axisRect.
    forEachAxisRect([](QCPAxisRect& rect)
    {
        forEachAxis(&rect, [](QCPAxis& axis) { axis.scaleRange(1.1); });
    });

    pQcp->replot();
}

template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, ValueCont_T&& data)
{
    // Note the terminology (using QCPGraph as example)
    //  QCPGraph().data() == QSharedPointer<QCPGraphDataContainer> -> DataContainer == QCPGraphDataContainer == QCPDataContainer<QCPGraphData>
    //  QCPGraph().data()->set() takes QVector<QCPGraphData> which is the actual storage, this is ValueCont_T
    using DataContainer = typename decltype(rChartObject.data())::value_type;
    QSharedPointer<DataContainer> spData(new DataContainer);
    std::sort(data.begin(), data.end(), qcpLessThanSortKey<typename ValueCont_T::value_type>);
    spData->set(data, true); // Note: if data is not sorted beforehand, sorting in set() will effetively cause a redundant copy to be created.
    rChartObject.setData(std::move(spData));
}

template <class DataType_T, class ChartObject_T, class Transform_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals, Transform_T transformer)
{
    QVector<DataType_T> values;
    const auto nSize = saturateCast<int>(Min(xVals.size(), yVals.size()));
    values.resize(nSize);
    // Copying from [x], [y] input to [(x,y)] storage that QCPStorage uses.
    transformer(values, xVals, yVals);
    fillQcpPlottable(rChartObject, std::move(values));
}

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals)
{
    using InputT = ::DFG_MODULE_NS(charts)::InputSpan<double>;
    const auto transformer = [](QVector<DataType_T>& dest, const InputT& xVals, const InputT& yVals) 
        {
            std::transform(xVals.cbegin(), xVals.cbegin() + dest.size(), yVals.cbegin(), dest.begin(), [](const double x, const double y) { return DataType_T(x, y); });
        };
    fillQcpPlottable<DataType_T>(rChartObject, xVals, yVals, transformer);
}

void fillQcpPlottable(QCPAbstractPlottable* pPlottable, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData)
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!pPlottable)
        return;
    const auto px = pipeData.constValuesByIndex(0);
    const auto py = pipeData.constValuesByIndex(1);
    if (!px || !py)
        return;
    auto pGraph = qobject_cast<QCPGraph*>(pPlottable);
    if (pGraph)
    {
        fillQcpPlottable<QCPGraphData>(*pGraph, makeRange(*px), makeRange(*py));
        return;
    }
    auto pCurve = qobject_cast<QCPCurve*>(pPlottable);
    if (pCurve)
    {
        using InputT = ::DFG_MODULE_NS(charts)::InputSpan<double>;
        const auto transformer = [](QVector<QCPCurveData>& dest, const InputT& xVals, const InputT& yVals)
        {
            for (int i = 0, nCount = dest.size(); i < nCount; ++i)
            {
                dest[i] = QCPCurveData(static_cast<double>(i), xVals[i], yVals[i]);
            }
        };
        fillQcpPlottable<QCPCurveData>(*pCurve, makeRange(*px), makeRange(*py), transformer);
        return;
    }
    auto pBars = qobject_cast<QCPBars*>(pPlottable);
    if (pBars)
    {
        fillQcpPlottable<QCPBarsData>(*pBars, makeRange(*px), makeRange(*py));
        return;
    }
    DFG_ASSERT_IMPLEMENTED(false);
}

#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// End of DFG_ALLOW_QCUSTOMPLOT
//
//////////////////////////////////////////////////////////////////////////////////////////////////////

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
        gConsoleLogHandle.setHandler([=](const QString& s, ConsoleLogLevel logLevel) { pConsole->addEntry(s, consoleLogLevelToEntryType(logLevel)); } );

        // Adding action for controlling cache detail logging.
        {
            auto pAction = new QAction(tr("Enable cache detail logging"), this); // Deletion through parentship.
            pAction->setCheckable(true);
            m_bLogCacheDiagnosticsOnUpdate = (DFG_BUILD_DEBUG_RELEASE_TYPE == StringViewC("debug"));
            pAction->setChecked(m_bLogCacheDiagnosticsOnUpdate);
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::toggled, [&](const bool b) { m_bLogCacheDiagnosticsOnUpdate = b; }));
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
    gConsoleLogHandle.setHandler(nullptr);
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

auto DFG_MODULE_NS(qt)::GraphControlPanel::getController() -> ChartController*
{
    return getControllerFromParents(this);
}

bool DFG_MODULE_NS(qt)::GraphControlPanel::getEnabledFlag() const
{
    auto pCbEnabled = findChild<const QCheckBox*>(QLatin1String("cb_enabled"));
    return (pCbEnabled) ? pCbEnabled->isChecked() : true;
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

namespace
{
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

    class GraphDisplayImageExportDialog : public QDialog
    {
    public:
        typedef QDialog BaseClass;
        typedef QJsonObject PropertyMap;
        enum class LogType { invalidParameter, error };
        GraphDisplayImageExportDialog(QCustomPlot* pCustomPlot);

        void accept() override;

        QByteArray getExportDefinition() const;

        void addLog(const LogType logType, const QString& sMsg);

        void onInvalidParameter(const QString& sMsg);

        QCP::ResolutionUnit stringToResolutionUnit(const QString& s);

        QPointer<QCustomPlot> m_spCustomPlot;
        ::DFG_MODULE_NS(qt)::QObjectStorage<QPlainTextEdit> m_spDefinitionEdit;
        ::DFG_MODULE_NS(qt)::QObjectStorage<QPlainTextEdit> m_spMessageConsole; // Guaranteed non-null between constructor and destructor.
    }; // class GraphDisplayImageExportDialog

    template <class T> T propertyValueAs(const QJsonObject&, const char*);
    template <> bool    propertyValueAs<bool>(const QJsonObject& obj, const char* psz)    { return obj.value(psz).toVariant().toBool(); }
    template <> int     propertyValueAs<int>(const QJsonObject& obj, const char* psz)     { return obj.value(psz).toVariant().toInt(); }
    template <> double  propertyValueAs<double>(const QJsonObject& obj, const char* psz)  { return obj.value(psz).toVariant().toDouble(); }
    template <> QString propertyValueAs<QString>(const QJsonObject& obj, const char* psz) { return obj.value(psz).toVariant().toString(); }

    GraphDisplayImageExportDialog::GraphDisplayImageExportDialog(QCustomPlot* pCustomPlot) :
        m_spCustomPlot(pCustomPlot)
    {
        m_spMessageConsole.reset(new QPlainTextEdit(this));
        m_spMessageConsole->setReadOnly(true);
        m_spMessageConsole->setMaximumHeight(100);

        DFG_MODULE_NS(qt)::removeContextHelpButtonFromDialog(this);

        // Definition widget
        {
            m_spDefinitionEdit.reset(new QPlainTextEdit(this));
            QVariantMap keyVals;
            keyVals.insert("format", "");
            keyVals.insert("type", "png");
            keyVals.insert("width", 0);
            keyVals.insert("height", 0);
            keyVals.insert("scale", 1.0);
            keyVals.insert("quality", -1);
            keyVals.insert("resolution", 96);
            keyVals.insert("resolution_unit", "dots_per_inch");
            keyVals.insert("pdf_title", "");
            const QString baseFileName = QString("dfgqte_image_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
            keyVals.insert("output_base_file_name", baseFileName);
            auto outputDir = QDir::current();
            if (!QFileInfo(outputDir.absolutePath()).isWritable()) // If current directory is not writable... 
                outputDir = QDir::home(); // ...changing output dir to user's home directory.
            keyVals.insert("output_dir", outputDir.absolutePath());

            // Params ignored for now:
            //keyVals.insert("pen", "TODO"); // pdf-specific
            //keyVals.insert("pdfCreator", "TODO"); // pdf-specific
            m_spDefinitionEdit->setPlainText(QJsonDocument::fromVariant(keyVals).toJson());
        }

        // Help widget
        auto pHelpWidget = new QLabel(this);
        pHelpWidget->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        pHelpWidget->setText(tr("<h3>Available image types and their parameters</h3>"
            "Note that most parameters are optional, required ones are underlined."
            "<ul>"
            "<li><b>bpm</b>: width, height, scale, resolution, resolution_unit</li>"
            "<li><b>pdf</b>: width, height, pdf_title</li>"
            "<li><b>png</b>: width, height, scale, quality, resolution, resolution_unit</li>"
            //"<li><b>jpg</b>: width, height, scale, quality, resolution, resolution_unit</li>" // Commented out for now as don't know when it would be a good idea to save chart-like content as jpg.
            "<li><b>rastered</b>: <u>format</u>, width, height, scale, quality, resolution, resolution_unit</li>"
            "</ul>"
            "<h3>Parameters</h3>"
            "<ul>"
            "<li><b>format</b>: Image format such as 'jpg', 'pbm', 'pgm', 'ppm', 'xbm' or 'xpm'. For more details, see Qt documentation for QImageWriter::supportedImageFormats().</li>"
            "<li><b>height</b>: Image height. For png, bmp, rastered: height in pixels. For pdf, see documentation of QCustomPlot::savePdf(). If this or width value is zero, uses widget dimensions.</li>"
            "<li><b>output_dir</b>: Directory where output file will be written.</li>"
            "<li><b>output_base_file_name</b>: Base name of output file Extension will be added automatically.</li>"
            "<li><b>output_overwrite</b>: If true, writes output file even if file at given path already exists.</li>"
            "<li><b>pdf_title</b>: Sets pdf title metadata.</li>"
            "<li><b>quality</b>: [0, 100], when -1, using default.</li>"
            "<li><b>resolution</b>: Written to image file header, no direct effect on quality or pixel size.</li>"
            "<li><b>resolution_unit</b>: Unit of 'resolution'-field. Possible values: dots_per_meter, dots_per_centimeter, dots_per_inch.</li>"
            "<li><b>scale</b>: Image scaling factor. For example if width is 100 and height 300, using scale factor 2 results to image of size 200*600. This will scale e.g. line widths accordingly.</li>"
            "<li><b>width</b>: Image width. Details are like for height.</li>"
            "</ul>"
        ));
        
        // Control buttons
        auto& rButtonBox = *(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this));

        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, &QDialogButtonBox::accepted, this, &GraphDisplayImageExportDialog::accept));
        DFG_QT_VERIFY_CONNECT(connect(&rButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject));

        // Adding items to layout.
        auto pLayout = new QVBoxLayout(this);
        pLayout->addWidget(m_spDefinitionEdit.get());
        pLayout->addWidget(pHelpWidget);
        pLayout->addWidget(m_spMessageConsole.get());
        pLayout->addWidget(&rButtonBox);

        setWindowTitle(tr("Image export"));
    }

    void GraphDisplayImageExportDialog::onInvalidParameter(const QString& sMsg)
    {
        addLog(LogType::invalidParameter, sMsg);
    }

    void GraphDisplayImageExportDialog::addLog(const LogType logType, const QString& sMsg)
    {
        QString sMsgWithTime = QString("%1: %2").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"), sMsg);
        if (logType == LogType::invalidParameter || logType == LogType::error)
            m_spMessageConsole->appendHtml(QString("<font color=\"#ff0000\">%1</font>").arg(sMsgWithTime));
        else
            m_spMessageConsole->appendHtml(sMsgWithTime);
    }

    QByteArray GraphDisplayImageExportDialog::getExportDefinition() const
    {
        return (m_spDefinitionEdit) ? m_spDefinitionEdit->toPlainText().toUtf8() : QByteArray();
    }

    QCP::ResolutionUnit GraphDisplayImageExportDialog::stringToResolutionUnit(const QString& s)
    {
        if (s == QLatin1String("dots_per_meter"))
            return QCP::ruDotsPerMeter;
        else if (s == QLatin1String("dots_per_centimeter"))
            return QCP::ruDotsPerCentimeter;
        else if (s == QLatin1String("dots_per_inch"))
            return QCP::ruDotsPerInch;
        else
        {
            onInvalidParameter(tr("Invalid resolution unit '%1'. Using dots_per_inch.").arg(s));
            return QCP::ruDotsPerInch;
        }
    }

    void GraphDisplayImageExportDialog::accept()
    {
        if (!m_spCustomPlot)
            return;

        const auto jsonDoc = QJsonDocument::fromJson(getExportDefinition());
        const auto propertyMap = jsonDoc.object();
        const auto sType = propertyValueAs<QString>(propertyMap, "type");

        const char* acceptedTypes[] = { "bmp", "pdf", "png", "rastered" };

        // Checking if type is valid.
        if (std::end(acceptedTypes) == std::find_if(std::begin(acceptedTypes), std::end(acceptedTypes), [&](const char* psz) { return QLatin1String(psz) == sType; }))
        {
            onInvalidParameter(tr("Invalid export type '%1'").arg(sType));
            return;
        }

        // Forming extension; uses type as such or in case of 'rastered', deducded from 'format'
        const QString sExtension = [&]()
            {
                if (sType != QLatin1String("rastered"))
                    return sType;
                else
                    return propertyValueAs<QString>(propertyMap, "format");
            }();

        // Making sure that extension is sane
        if (sExtension.isEmpty() || sExtension.size() > 4 || std::any_of(sExtension.begin(), sExtension.end(), [](const QChar& c) { return !c.isLetterOrNumber() || c.unicode() > 127; }))
        {
            onInvalidParameter(tr("Extension '%1' is not accepted; for now maximum extension length is 4 and only ASCII-letters and numbers are accepted").arg(sExtension));
            return;
        }

        const auto sOutputDir = propertyValueAs<QString>(propertyMap, "output_dir");
        const auto sOutputBaseFileName = propertyValueAs<QString>(propertyMap, "output_base_file_name");
        if (sOutputDir.isEmpty() || sOutputBaseFileName.isEmpty())
        {
            onInvalidParameter(tr("Unable to export image: output directory path or base file name is empty"));
            return;
        }
        const QString sFileName = QString("%1.%2").arg(sOutputBaseFileName, sExtension);
        const QString sOutputPath = QDir(sOutputDir).absoluteFilePath(sFileName);
        const QFileInfo fileInfo(sOutputPath);
        if (fileInfo.isDir())
        {
            onInvalidParameter(tr("Unable to write to path '%1': path is an existing directory").arg(sOutputPath));
            return;
        }
        if (fileInfo.exists() && !propertyValueAs<bool>(propertyMap, "output_overwrite"))
        {
            onInvalidParameter(tr("Unable to write to path'%1': file already exists. If overwriting is desired, set 'output_overwrite' to true.").arg(sOutputPath));
            return;
        }

        bool bSuccess = false;
        if (sType == "bmp")
        {
            bSuccess = m_spCustomPlot->saveBmp(sOutputPath,
                propertyValueAs<int>(propertyMap, "width"),
                propertyValueAs<int>(propertyMap, "height"),
                propertyValueAs<double>(propertyMap, "scale"),
                propertyValueAs<int>(propertyMap, "resolution"),
                stringToResolutionUnit(propertyValueAs<QString>(propertyMap, "resolution_unit"))
            );
        }
        else if (sType == "pdf")
        {
            bSuccess = m_spCustomPlot->savePdf(sOutputPath,
                propertyValueAs<int>(propertyMap, "width"),
                propertyValueAs<int>(propertyMap, "height"),
                QCP::epAllowCosmetic, // TODO: make customisable.
                QString(), // TODO: check what to use (is 'pdfCreator'-field)
                propertyValueAs<QString>(propertyMap, "pdf_title")
            );
        }
        else if (sType == "png")
        {
            bSuccess = m_spCustomPlot->savePng(sOutputPath,
                propertyValueAs<int>(propertyMap, "width"),
                propertyValueAs<int>(propertyMap, "height"),
                propertyValueAs<double>(propertyMap, "scale"),
                propertyValueAs<int>(propertyMap, "quality"),
                propertyValueAs<int>(propertyMap, "resolution"),
                stringToResolutionUnit(propertyValueAs<QString>(propertyMap, "resolution_unit"))
                );
        }
        else if (sType == "rastered")
        {
            bSuccess = m_spCustomPlot->saveRastered(sOutputPath,
                propertyValueAs<int>(propertyMap, "width"),
                propertyValueAs<int>(propertyMap, "height"),
                propertyValueAs<double>(propertyMap, "scale"),
                propertyValueAs<QString>(propertyMap, "format").toUtf8(),
                propertyValueAs<int>(propertyMap, "quality"),
                propertyValueAs<int>(propertyMap, "resolution"),
                stringToResolutionUnit(propertyValueAs<QString>(propertyMap, "resolution_unit"))
            );
        }
        else // Case: unknown type
        {
            onInvalidParameter(tr("Invalid type '%1'").arg(sType));
            return;
        }

        if (bSuccess)
        {
            QMessageBox::information(this, nullptr, tr("Successfully exported to path<br>") + QString("<a href=file:///%1>%1</a>").arg(sOutputPath));
        }
        else
        {
            addLog(LogType::error, tr("Export failed."));
            return;
        }

        BaseClass::accept();
    }

#endif // defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
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
        menu.addAction(tr("Rescale all axis"), [=]() { auto pChart = this->chart(); if (pChart) pChart->optimizeAllAxesRanges(); });

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

DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::GraphControlAndDisplayWidget()
{
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

auto DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::defaultSourceIdImpl() const -> GraphDataSourceId 
{
    return this->m_sDefaultDataSource;
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::clearCachesImpl()
{
    m_spCache.reset();
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshImpl()
{
    using namespace ::DFG_MODULE_NS(charts);
    DFG_MODULE_NS(time)::TimerCpu timer;

    if (m_spCache)
        m_spCache->removeInvalidCaches();

    // TODO: graph filling should be done in worker thread to avoid GUI freezing.
    //       It's worth noting though that data sources might not be thread-safe (at least CsvItemModelChartDataSource wasn't as of 2020-06-29).
    //       Data sources should probably either guarantee thread-safety or at least be able to gracefully handle threading:
    //       e.g. provide interface from which user of data source can check if certain operations can be done and/or data source should ignore
    //       read requests that can't be done in safe manner.

    if (!m_spGraphDisplay)
    {
        DFG_ASSERT_WITH_MSG(false, "Internal error, no graph display object");
        return;
    }
    auto pChart = m_spGraphDisplay->chart();
    if (!pChart)
    {
        DFG_ASSERT_WITH_MSG(false, "Internal error, no chart object available");
        return;
    }

    auto& rChart = *pChart;

    // Clearing existing objects.
    rChart.removeAllChartObjects();

    auto pDefWidget = getDefinitionWidget();
    if (!pDefWidget)
    {
        DFG_QT_CHART_CONSOLE_ERROR("Internal error: missing control widget");
        return;
    }

    int nGraphCounter = 0;
    int nHistogramCounter = 0;
    int nBarsCounter = 0;
    GraphDefinitionEntry globalConfigEntry; // Stores global config entry if present.
    GraphDefinitionEntry* pGlobalConfigEntry = nullptr; // Set to point to global config if such exists. This and globalConfigEntry are nothing but inconvenient way to do what optional would provide.
    ::DFG_MODULE_NS(cont)::MapVectorAoS<StringUtf8, GraphDefinitionEntry> mapPanelIdToConfig;
    // Going through every item in definition entry table and redrawing them.
    pDefWidget->getChartDefinition().forEachEntry([&](const GraphDefinitionEntry& defEntry)
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
                gConsoleLogHandle.setDesiredLevel(globalConfigEntry.logLevel());
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

            if (sEntryType == ChartObjectChartTypeStr_xy || sEntryType == ChartObjectChartTypeStr_txy)
                refreshXy(rChart, configParamCreator, source, defEntry, nGraphCounter);
            else if (sEntryType == ChartObjectChartTypeStr_histogram)
                refreshHistogram(rChart, configParamCreator, source, defEntry, nHistogramCounter);
            else if (sEntryType == ChartObjectChartTypeStr_bars)
                refreshBars(rChart, configParamCreator, source, defEntry, nBarsCounter);
            else
            {
                if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                    defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("missing handler for type '%1'").arg(sEntryType.c_str()));
                return;
            }
        });
    });
    rChart.optimizeAllAxesRanges();

    if (ChartConfigParam(pGlobalConfigEntry).value(ChartObjectFieldIdStr_showLegend, pChart->isLegendEnabled()))
    {
        pChart->enableLegend(true);
        pChart->createLegends();
    }
    else
        pChart->enableLegend(false);

    DFG_MODULE_NS(time)::TimerCpu timerRepaint;
    rChart.repaintCanvas();
    const auto elapsedRepaint = timerRepaint.elapsedWallSeconds();
    const auto elapsed = timer.elapsedWallSeconds();
    DFG_QT_CHART_CONSOLE_INFO(tr("Refresh lasted %1 ms (repaint took %2 %)").arg(round<int>(1000 * elapsed)).arg(round<int>(100.0 * elapsedRepaint / elapsed)));

    // Cache diagnostics
    if (m_spCache && m_spControlPanel->m_bLogCacheDiagnosticsOnUpdate)
    {
        QString s;
        for (const auto& item : m_spCache->m_tableSelectionDatas)
        {
            if (!s.isEmpty())
                s += ", ";
            s += QString("{ key: %1, column count: %2 }").arg(item.first).arg(item.second->columnCount());
        }
        DFG_QT_CHART_CONSOLE_INFO(QString("Cache usage: %1").arg(s));
    }
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
    void makeEffectiveColumnIndexes(GraphDataSource::DataSourceIndex& x, GraphDataSource::DataSourceIndex& y, const ColRange_T& colRange, IsRowSpecifier_T isRowIndexSpecifier)
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

        if (isPresent(x)) // Case: x was defined
        {
            if (!isPresent(y)) // y is not set?
            {
                if (isRowIndexSpecifier(y))
                    y = x;
                else
                    y = colRange[(indexOf(x) + 1) % colRange.size()]; // Using next column as y (wrapping to beginning if x is last).
            }
        }
        else if (isPresent(y)) // case: x is not defined and y defined
        {
            if (isRowIndexSpecifier(x))
                x = y;
            else
            {
                const auto indexOfy = indexOf(y);
                if (colRange.size() >= 2)
                    x = (indexOfy > 0) ? colRange[indexOfy - 1] : colRange.back(); // Using one before y-column as x (wrapping to end if y is first)
                else // Case: only one column -> x = y
                    x = y;
            }
        }
        else // Case: both are either undefined or other one is row_index-specifier
        {
            const auto nDefaultYindex = (colRange.size() >= 2) ? 1 : 0;
            if (isRowIndexSpecifier(x))
            {
                y = colRange[nDefaultYindex];
                x = y;
            }
            else if (isRowIndexSpecifier(y))
            {
                x = colRange[0];
                y = x;
            }
            else
            {
                //using defaults i.e.first column as x and next (if available) as y
                x = colRange[0];
                y = colRange[nDefaultYindex];
            }
        }
    }
} } } // dfg:::qt::<unnamed>

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshXy(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nGraphCounter)
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!m_spCache)
        m_spCache.reset(new ChartDataCache);

    std::array<DataSourceIndex, 2> columnIndexes;
    std::array<bool, 2> rowFlags;
    auto optData = m_spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optData || optData->columnCount() < 1)
        return;

    auto& tableData = *optData;

    auto pXdata = tableData.columnDataByIndex(columnIndexes[0]);
    auto pYdata = tableData.columnDataByIndex(columnIndexes[1]);

    if (!pXdata || !pYdata)
        return;

    const bool bXisRowIndex = rowFlags[0];
    const bool bYisRowIndex = rowFlags[1];

    const char szRowIndexName[] = QT_TR_NOOP("Row number");
    const auto xType = (!bXisRowIndex) ? tableData.columnDataType(pXdata) : ChartDataType(ChartDataType::unknown);
    const auto yType = (!bYisRowIndex) ? tableData.columnDataType(pYdata) : ChartDataType(ChartDataType::unknown);
    const auto sXname = (!bXisRowIndex) ? tableData.columnName(pXdata) : QString(szRowIndexName);
    const auto sYname = (!bYisRowIndex) ? tableData.columnName(pYdata) : QString(szRowIndexName);
    auto spSeries = rChart.createXySeries(XySeriesCreationParam(nGraphCounter++, configParamCreator(), defEntry, xType, yType, qStringToStringUtf8(sXname), qStringToStringUtf8(sYname)));
    if (!spSeries)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("couldn't create series object"));
        return;
    }
    auto& rSeries = *spSeries;

    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;

    // xValueMap is also used as final (x,y) table passed to series.
    // releaseOrCopy() will return either moved data or copy of it.
    auto xValueMap = (pXdata != pYdata) ? tableData.releaseOrCopy(pXdata) : *pXdata;
    xValueMap.setSorting(false);

    using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<int>;
    IntervalSet xRows;
    const IntervalSet* pxRowSet = (!xValueMap.empty()) ? defEntry.createXrowsSet(xRows, static_cast<int>(xValueMap.backKey() + 1)) : nullptr;
    if (pxRowSet)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::debug))
            defEntry.log(GraphDefinitionEntry::LogLevel::debug, tr("x_rows-entry defines %1 row(s)").arg(xRows.sizeOfSet()));
    }

    const auto& yValueMap = *pYdata;
    auto xIter = xValueMap.cbegin();
    auto yIter = yValueMap.cbegin();
    DataSourceIndex nSizeAfterRowFilter = 0;
    for (; xIter != xValueMap.cend() && yIter != yValueMap.cend();)
    {
        const auto xRow = xIter->first;
        const auto yRow = yIter->first;
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
        // Current xIter and yIter point to the same row ->
        // store (x,y) values to start of xValueMap if not filtered out.
        if (!pxRowSet || pxRowSet->hasValue(static_cast<int>(xRow))) // Trusting xRow to have value that can be casted to int.
        {
            const auto x = (bXisRowIndex) ? xRow : xIter->second;
            const auto y = (bYisRowIndex) ? yRow : yIter->second;

            if (!::DFG_MODULE_NS(math)::isNan(x)) // Accepting point only if x is non-NaN
            {
                xValueMap.m_keyStorage[nSizeAfterRowFilter] = x;
                xValueMap.m_valueStorage[nSizeAfterRowFilter] = y;
                minMaxX(x);
                minMaxY(y);
                nSizeAfterRowFilter++;
            }
        }
        ++xIter;
        ++yIter;
    }

    xValueMap.m_keyStorage.resize(nSizeAfterRowFilter);
    xValueMap.m_valueStorage.resize(nSizeAfterRowFilter);

    // Applying operations
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&xValueMap.m_keyStorage, &xValueMap.m_valueStorage);
    defEntry.applyOperations(operationData);

    auto pXvalues = operationData.constValuesByIndex(0);
    auto pYvalues = operationData.constValuesByIndex(1);

    if (!pXvalues || !pYvalues)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available after operations"));
        return;
    }

    // Setting values to series.
    rSeries.setValues(makeRange(*pXvalues), makeRange(*pYvalues));

    // Setting line style
    {
        bool bFound = false;
        defEntry.doForLineStyleIfPresent([&](const char* psz) { bFound = true; rSeries.setLineStyle(psz); });
        if (!bFound) // If not found, setting value from default config or if not present there either, using basic-style
            rSeries.setLineStyle(configParamCreator().valueStr(ChartObjectFieldIdStr_lineStyle, SzPtrUtf8(ChartObjectLineStyleStr_basic)));
    }

    // Setting point style
    {
        bool bFound = false;
        defEntry.doForPointStyleIfPresent([&](const char* psz) { bFound = true; rSeries.setPointStyle(psz); });
        if (!bFound) // If not found, setting value from default config or if not present there either, using basic-style
            rSeries.setPointStyle(configParamCreator().valueStr(ChartObjectFieldIdStr_pointStyle, SzPtrUtf8(ChartObjectPointStyleStr_basic)));
    }

    setCommonChartObjectProperties(rSeries, defEntry, configParamCreator, DefaultNameCreator("Graph", nGraphCounter, sXname, sYname));
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

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshHistogram(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nHistogramCounter)
{
    using namespace ::DFG_MODULE_NS(charts);
    const auto nColumnCount = source.columnCount();
    if (nColumnCount < 1)
        return;

    if (!m_spCache)
        m_spCache.reset(new ChartDataCache);

    const auto sBinType = defEntry.fieldValueStr(ChartObjectFieldIdStr_binType, [] { return StringUtf8(DFG_UTF8("number")); });

    if (sBinType != DFG_UTF8("number") && sBinType != DFG_UTF8("text"))
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("Unrecognized bin_type '%1', using default").arg(sBinType.c_str().c_str()));
    }

    const bool bTextValued = (sBinType == DFG_UTF8("text"));

    std::array<DataSourceIndex, 2> columnIndexes;
    std::array<bool, 2> rowFlags;
    auto optTableData = m_spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optTableData)
        return;

    ChartObjectHolder<Histogram> spHistogram;
    if (!bTextValued)
    {
        if (!optTableData || optTableData->columnCount() < 1)
            return;

        auto pCacheColumn = optTableData->columnDataByIndex(columnIndexes[0]);

        if (!pCacheColumn)
            return;

        TableSelectionCacheItem::RowToValueMap singleColumnCopy;
        const TableSelectionCacheItem::RowToValueMap* pRowToValues = pCacheColumn;

        if (!handleXrows(defEntry, pRowToValues, singleColumnCopy))
            return;

        // Applying operations
        ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&pRowToValues->m_valueStorage, nullptr);
        defEntry.applyOperations(operationData);

        auto pValues = operationData.constValuesByIndex(0);
        if (!pValues || pValues->empty())
        {
            if (!pValues && defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available after operations"));
            return;
        }

        const auto xType = optTableData->columnDataType(pCacheColumn);
        const auto sXaxisName = optTableData->columnName(pCacheColumn);
        spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, makeRange(*pValues), xType, qStringToStringUtf8(sXaxisName)));

    }
    else // Case text valued
    {
        auto pStrings = optTableData->columnStringsByIndex(columnIndexes[0]);
        if (!pStrings || pStrings->empty())
        {
            if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::error))
                defEntry.log(GraphDefinitionEntry::LogLevel::error, tr("no data found for histogram"));
            return;
        }

        TableSelectionCacheItem::RowToStringMap stringColumnCopy;

        if (!handleXrows(defEntry, pStrings, stringColumnCopy))
            return;
        
        // Applying operations
        ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&pStrings->m_valueStorage, nullptr);
        defEntry.applyOperations(operationData);

        auto pFinalStrings = operationData.constStringsByIndex(0);
        if (!pFinalStrings || pFinalStrings->empty())
        {
            if (!pStrings && defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
                defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no values available after operations"));
            return;
        }

        const auto sXaxisName = optTableData->columnName(columnIndexes[0]);
        spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, makeRange(*pFinalStrings), qStringToStringUtf8(sXaxisName)));
    }

    if (!spHistogram)
        return;
    ++nHistogramCounter;
    setCommonChartObjectProperties(*spHistogram, defEntry, configParamCreator, DefaultNameCreator("Histogram", nHistogramCounter));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshBars(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nBarsCounter)
{
    using namespace ::DFG_MODULE_NS(charts);

    const auto nColumnCount = source.columnCount();
    if (nColumnCount < 1)
        return;

    if (!m_spCache)
        m_spCache.reset(new ChartDataCache);

    std::array<DataSourceIndex, 2> columnIndexes;
    std::array<bool, 2> rowFlags;
    auto optTableData = m_spCache->getTableSelectionData_createIfMissing(source, defEntry, columnIndexes, rowFlags);

    if (!optTableData || optTableData->columnCount() < 1)
        return;

    auto pFirstCol = optTableData->columnStringsByIndex(columnIndexes[0]);
    auto pSecondCol = optTableData->columnDataByIndex(columnIndexes[1]);

    if (!pFirstCol || !pSecondCol)
        return;

    TableSelectionCacheItem::RowToStringMap labelColumnCopy;
    TableSelectionCacheItem::RowToValueMap valueColumnCopy;
    // Applying x_rows filter if defined
    {
        if (!handleXrows(defEntry, pFirstCol, labelColumnCopy))
            return;
        if (!handleXrows(defEntry, pSecondCol, valueColumnCopy))
            return;
    }

    const auto sXaxisName = optTableData->columnName(columnIndexes[0]);
    const auto sYaxisName = optTableData->columnName(columnIndexes[1]);

    // Applying operations
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData operationData(&pFirstCol->m_valueStorage, &pSecondCol->m_valueStorage);
    defEntry.applyOperations(operationData);

    auto pStrings = operationData.constStringsByIndex(0);
    auto pValues = operationData.constValuesByIndex(1);
    if (!pStrings || !pValues)
    {
        if (defEntry.isLoggingAllowedForLevel(GraphDefinitionEntry::LogLevel::warning))
            defEntry.log(GraphDefinitionEntry::LogLevel::warning, tr("no data available after operations"));
        return;
    }

    auto spBarSeries = rChart.createBarSeries(BarSeriesCreationParam(configParamCreator(), defEntry, makeRange(*pStrings), makeRange(*pValues), ChartDataType::unknown,
                                                                     qStringToStringUtf8(sXaxisName), qStringToStringUtf8(sYaxisName)));
    if (!spBarSeries)
        return;
    ++nBarsCounter;
    setCommonChartObjectProperties(*spBarSeries, defEntry, configParamCreator, DefaultNameCreator("Bars", nBarsCounter));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::setCommonChartObjectProperties(ChartObject& rObject, const GraphDefinitionEntry& defEntry, ConfigParamCreator configParamCreator, const DefaultNameCreator& defaultNameCreator)
{
    rObject.setName(defEntry.fieldValueStr(ChartObjectFieldIdStr_name, defaultNameCreator));
    rObject.setLineColour(defEntry.fieldValueStr(ChartObjectFieldIdStr_lineColour, [&] { return configParamCreator().valueStr(ChartObjectFieldIdStr_lineColour); }));
    rObject.setFillColour(defEntry.fieldValueStr(ChartObjectFieldIdStr_fillColour, [&] { return configParamCreator().valueStr(ChartObjectFieldIdStr_fillColour); }));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::handlePanelProperties(ChartCanvas& rChart, const GraphDefinitionEntry& defEntry)
{
    const auto sPanelId = defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId);
    rChart.setTitle(sPanelId, defEntry.fieldValueStr(ChartObjectFieldIdStr_title));

    rChart.setAxisLabel(sPanelId, DFG_UTF8("x"), defEntry.fieldValueStr(ChartObjectFieldIdStr_xLabel));
    rChart.setAxisLabel(sPanelId, DFG_UTF8("y"), defEntry.fieldValueStr(ChartObjectFieldIdStr_yLabel));

    rChart.setAxisTickLabelDirection(sPanelId, DFG_UTF8("x"), defEntry.fieldValueStr(ChartObjectFieldIdStr_xTickLabelDirection));
    rChart.setAxisTickLabelDirection(sPanelId, DFG_UTF8("y"), defEntry.fieldValueStr(ChartObjectFieldIdStr_yTickLabelDirection));
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
    QTimer::singleShot(0, [&]() { refresh(); m_bRefreshPending = false; });
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onDataSourceDestroyed()
{
    //refresh();
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)> func)
{
    auto iter = std::find_if(m_dataSources.m_sources.begin(), m_dataSources.m_sources.end(), [&](const std::shared_ptr<GraphDataSource>& spDs)
    {
        return (spDs && spDs->uniqueId() == id);
    });

    // If source wasn't found, checking if it's file source and creating new source if needed.
    if (iter == m_dataSources.m_sources.end())
    {
        const auto sId = qStringToStringUtf8(id);
        ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem item(sId);
        if (item.key() == DFG_UTF8("csv_file"))
        {
            auto spCsvSource = std::make_shared<CsvFileDataSource>(viewToQString(item.value(0)), id);
            m_dataSources.m_sources.push_back(std::move(spCsvSource));
            iter = m_dataSources.m_sources.end() - 1;
        }
        else if (item.key() == DFG_UTF8("sqlite_file"))
        {
            auto spSqliteSource = std::make_shared<SQLiteFileDataSource>(viewToQString(item.value(1)), viewToQString(item.value(0)), id);
            m_dataSources.m_sources.push_back(std::move(spSqliteSource));
            iter = m_dataSources.m_sources.end() - 1;
        }
    }

    if (iter != m_dataSources.m_sources.end())
        func(**iter);
    else
        DFG_QT_CHART_CONSOLE_WARNING(tr("Didn't find data source '%1', available sources (%2 in total): { %3 }")
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
