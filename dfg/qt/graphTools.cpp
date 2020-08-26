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

#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
    #include <QtCharts>
    #include <QtCharts/QChart>
    #include <QtCharts/QChartView>
    #include <QtCharts/QLineSeries>
    #include <QtCharts/QDateTimeAxis>
    #include <QtCharts/QValueAxis>
#endif

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

using namespace DFG_MODULE_NS(charts)::fieldsIds;

#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
DFG_ROOT_NS_BEGIN{
    // QCPDataContainer doesn't seem to have value_type typedef, so adding specializations to ElementType<> so that QCPDataContainer works with nearestRangeInSorted()
    DFG_SUB_NS(cont)
    {
        namespace DFG_DETAIL_NS
        {
            template <> struct ElementTypeFromConstReferenceRemoved<QCPGraphDataContainer> { typedef QCPGraphData type; };
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

enum class ConsoleLogLevel
{
    debug,
    info,
    warning,
    error,
    none
}; // ConsoleLogLevel


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
        if (m_handler)
            m_effectiveLevel = m_desiredLevel;
        else
            m_effectiveLevel = ConsoleLogLevel::none;
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

    ConsoleLogLevel effectiveLevel() const { return m_effectiveLevel; }

    ConsoleLogLevel m_effectiveLevel = ConsoleLogLevel::none;
    ConsoleLogLevel m_desiredLevel = ConsoleLogLevel::info;
    HandlerT m_handler = nullptr;
};

static ConsoleLogHandle gConsoleLogHandle;

} // unnamed namespace

#define DFG_QT_CHART_CONSOLE_LOG(LEVEL, MSG) if (LEVEL >= gConsoleLogHandle.effectiveLevel()) gConsoleLogHandle.log(MSG, LEVEL)
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
    const char* psz = sv.begin();
    psz = ::DFG_MODULE_NS(str)::skipWhitespacesSz(psz, " \t");
    if (*psz != '\0' && ::DFG_MODULE_NS(alg)::contains("+-.0123456789", *psz)) // If first non-empty char is any of listed, trying to convert string to double.
        return ::DFG_MODULE_NS(str)::strTo<double>(sv);
    else
        return std::numeric_limits<double>::quiet_NaN();
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
    if (std::count(s.begin(), s.end(), '-') >= 2 || std::count(s.begin(), s.end(), '.') >= 2 || std::count(s.begin(), s.end(), ':') >= 2)
        return std::numeric_limits<double>::quiet_NaN();
    if (std::find(s.begin(), s.end(), ',') == s.end())
        return stringToDouble(StringViewSzC(svUtf8.beginRaw())); // Not using s directly because at the time of writing it caused redundant string-object to be created due to shortcomings in strTo().
    else
    {
        auto s2 = viewToQString(s);
        s2.replace(',', '.'); // Hack: to make comma-localized values such as "1,2" be interpreted as 1.2
        return stringToDouble(s2);
    }
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
    using StringViewOrOwner = ::DFG_ROOT_NS::StringViewOrOwner<StringViewSzC, std::string>;
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

    // Returns graph type as string. String view is guaranteed valid for lifetime of *this.
    StringViewOrOwner graphTypeStr() const;

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

    ::DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, Operation> m_operationMap;

private:
    QJsonValue getField(FieldIdStrViewInputParam fieldId) const; // fieldId must be a ChartObjectFieldIdStr_

    std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const override;
    void forEachPropertyIdImpl(std::function<void(StringView svId)>) const override;

    template <class Func_T>
    void doForFieldIfPresent(FieldIdStrViewInputParam id, Func_T&& func) const;


    QJsonDocument m_items;
    int m_nContainerIndex = -1;

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

    // Reading operations
    {
        ::DFG_MODULE_NS(charts)::ChartEntryOperationManager manager;
        manager.setStringToDoubleConverter([](const StringViewSzUtf8 sv) { return GraphDataSource::cellStringToDouble(sv, 0, nullptr); });
        rv.forEachPropertyId([&](const StringViewUtf8& svKey)
        {
            if (!::DFG_MODULE_NS(str)::beginsWith(svKey, StringViewUtf8(SzPtrUtf8(ChartObjectFieldIdStr_operation))))
                return;
            const auto sOperationDef = rv.fieldValueStr(StringViewC(svKey.beginRaw(), svKey.endRaw()));
            auto op = manager.createOperation(sOperationDef);
            if (!op)
            {
                DFG_QT_CHART_CONSOLE_WARNING(QCoreApplication::tr("Unable to create '%1: %2'")
                    .arg(viewToQString(svKey))
                    .arg(viewToQString(sOperationDef)));
            }
            else
            {
                StringViewUtf8 svOperationOrderTag(SzPtrUtf8(svKey.beginRaw() + ::DFG_MODULE_NS(str)::strLen(ChartObjectFieldIdStr_operation)), SzPtrUtf8(svKey.endRaw()));
                rv.m_operationMap[svOperationOrderTag.toString()] = std::move(op);
            }
        });
    }

    return rv;
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
        DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: Both columns are specified as row_index").arg(defEntry.index()));
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
    return tr(R"ENDTAG(
<h2>Overview:</h2>
<ul>
    <li>Charts are defined as a list of single line JSON-entries. One entry typically defines one graph.
    <li>Entries are evaluated in order from top to bottom.
    <li>All entries have a <i>type</i> that defines list of accepted fields.
    <li>List can have comments: lines beginning with # are not parsed.
</ul>

<h2>Basic examples:</h2>
<ul>
    <li>Setting default config: {"type":"global_config","show_legend":true,"auto_axis_labels":true}</li>
    <li>Basic graph: {"type":"xy"}
    <li>Basic graph with lines and points: {"line_style":"basic","point_style":"basic","type":"xy"}
    <li>Basic graph with most options present: {"enabled":true,"line_style":"basic","point_style":"basic","line_colour":"#7fFF00FF","type":"xy","name":"Example graph","x_source":"column_name(column 1)", "y_source":"column_name(column 3)", "x_rows":"1:3; 5; 7:8", "panel_id":"grid(2,2)"}
    <li>Basic histogram: {"type":"histogram","name":"Basic histogram"}
    <li>Setting panel title and axis labels: {"type":"panel_config","panel_id":"grid(1,1)","title":"Title for\npanel (1,1)","x_label":"This is x axis label","y_label":"This is y axis label"}
    <li>Histogram from source named 'table': {"type":"histogram","data_source":"table","name":"Basic histogram"}
    <li>Basic bar chart: {"type":"bars"}
    <li>Disabling an entry by commenting: #{"type":"histogram","name":"Basic histogram"}
    <li>Filtering out points whose y value is not within [5, 10]: {"type":"xy","operation_a":"passWindow(y, 5, 10)"}
</ul>

<h2>Common fields</h2>
<ul>
   <li><i>enabled</i>: {<b>true</b>, false}. Defines whether entry is enabled, default value is true.</li>
    <ul>
        <li>Entry can also be disabled by beginning line with #. When commented, the line is not parsed at all.</li>
    </ul>
   <li><i>type</i>: Defines entry type</li>
    <ul>
        <li>xy              : Graph of (x, y) points shown sorted by x-value. When only one column is available, uses line numbers as x-values</li>
        <li>histogram       : Histogram</li>
        <li>bars            : Bar chart, especially for showing bars for data where one column defines labels (strings) and another column specifies values for labels.</li>
        <li>panel_config    : Defines panel items such as title and axes labels</li>
        <li>global_config   : Defines default config values for all panels.</li>
    </ul>
   <li><i>name</i>: name of the object, shown e.g. in legend.</li>
   <li><i>panel_id</i>: Target panel (e.g. panel where graph is drawn). Currently grid paneling is supported; syntax is "panel_id":"grid(row number, column number)". (1,1) means top left.</li>
   <li><i>data_source</i>: Defines data source from which to fetch data. If omitted, default source is used.</li>
</ul>

<h2>Fields for type <i>xy</i></h2>
    <ul>
        <li><i>x_source</i>:</li>
            <ul>
                <li>column_name(name): x-values will be taken from column that has name <i>name</i>.</li>
                <li>row_index: values will be taken from row index of y-values</li>
            </ul>
        <li><i>y_source</i>:</li>
            <ul>
                <li>column_name(name): y-values will be taken from column that has name <i>name</i>.</li>
                <li>row_index: values will be taken from row index of x-values</li>
            </ul>
        <li><i>x_rows</i>:</li>
            <ul>
                <li>x_rows: List of rows to include as semicolon separated list, e.g. "1:3; 4; 7:8" means 1, 2, 3, 4, 7, 8.</li>
                <li>Note that in case of filtered tables, indexes refer to visible row, not the row ID shown in the row header</li>
            </ul>
        <li><i>line_colour</i>:</li>
            <ul>
                <li>Sets line colour for drawn graph line. Also affects point marker colour.</li>
                <li>Accepted format as defined in QColor::setNamedColor()
                    <ul>
                        <li>#RGB (each item is a single hex digit)</li>
                        <li>#RRGGBB (each as two digit hex)</li>
                        <li>#AARRGGBB (AA is alpha channel that specifies transparency, 00 means fully transparent and ff fully opaque.)</li>
                        <li>#RRRGGGBBB</li>
                        <li>#RRRRGGGGBBBB</li>
                        <li>Item from list of SVG color keyword names by World Wide Web Consortium, for example 'red', green', 'blue'</li>
                    </ul>
                <li>example: line_colour: #7f1b2eff). Sets semi-transparent, mostly blue line.</li>
            </ul>
        <li><i>line_style</i>:</li>
            <ul>
                <li>none:        No line indicator</li>
                <li><b>basic</b>:       Straight line between data points. Default value if field not present</li>
                <li>step_left:   Horizontal line between points, y-value from left point.</li>
                <li>step_right:  Horizontal line between points, y-value from right point.</li>
                <li>step_middle: Horizontal line around point, steps at midway on both sides.</li>
                <li>pole:        Vertical line from (x, 0) -> (x, y) for each point.</li>
            </ul>
        <li><i>point_style</i>:</li>
            <ul>
                <li><b>none</b> : No point indicator. Default value if field not present</li>
                <li>basic: Basic indicator for every point</li>
            </ul>
        <li>Operations, see Operations-section for details</li>
    </ul>
<h2>Fields for type <i>histogram</i></h2>
    <ul>
        <li><i>bin_count</i>: Number of bins in histogram. With value -1, every value gets it's own bin. (Default is currently 100, but this may change so it is not to be relied on)</li>
        <li><i>bar_width_factor</i>: Defines visual spacing between bars as ratio how wide visual bar is compared to bin: value 1 means that bars are as wide as bins and there's no empty space between adjacent bars. With value 0 bars are sharp peaks. Default value is 1. Note that value can also be > 1.</li>
            <ul>
                <li>Note: Does not work properly when combined with bin_count = -1 and data where distance between adjacent data point varies.</li>
            </ul>
        <li><i>x_source</i>: Defines column from which histogram is created, usage like described in xy-type. If omitted, uses first column.
        <li><i>line_colour</i>: sets line colour, for details, see documentation in type <i>xy</i>.</li>
        <li><i>fill_colour</i>: sets bar fill colour, syntax details like for line_colour.</li>
        <li><i>bin_type</i>: {<b>number</b>, text}. Defines how input is interpreted; with 'text', there's one bar for each unique text and value is the number of it's occurrences.</li>
    </ul>
<h2>Fields for type <i>bars</i></h2>
    <ul>
        <li><i>line_colour</i>: sets line colour, for details, see documentation in type <i>xy</i>.</li>
        <li><i>fill_colour</i>: sets bar fill colour, syntax details like for line_colour.</li>
        <li><i>x_source</i>: defines column from which labels are taken from, see syntax from documentation of type <i>xy</i>. <i>row_index</i> specifier is not supported.</li>
        <li><i>y_source</i>: defines column from which values are taken from, see syntax from documentation of type <i>xy</i>. <i>row_index</i> specifier is not supported.</li>
        <li><i>merge_identical_labels</i>: {true, <b>false</b>}. If true, bars with identical labels are merged into one and bar length is the sum of lengths of merged bars.</li>
    </ul>
<h2>Fields for type <i>panel_config</i></h2>
    <ul>
        <li><i>title</i>: Panel title. New lines can be added with \n</li>
        <li><i>x_label</i>: Label of x-axis. New lines can be added with \n</li>
        <li><i>y_label</i>: Label of y-axis. New lines can be added with \n</li>
        <li><i>x_tick_label_direction</i>: Direction of tick labels on x-axis by angle [-90, 90]. 0 = horizontal, positive is clockwise rotation.</li>
        <li><i>y_tick_label_direction</i>: Like x_tick_label_direction, but for y-axis.</li>
    </ul>
<h2>Fields for type <i>global_config</i></h2>
    <ul>
        <li><i>show_legend</i>: {true, <b>false</b>}. Enables or disables showing legends, sets default behaviour for all panels.</li>
        <li><i>auto_axis_labels</i>: {<b>true</b>, false}. Enables or disables auto axis labels, sets default behaviour for all panels.</li>
        <li><i>line_style</i>: Sets global default for line_style, see line style documentation for details.</li>
        <li><i>point_style</i>: Sets global default for point_style, see point style documentation for details.</li>
        <li><i>line_colour</i>: Sets global default for line_colour, see line colour documentation for details.</li>
        <li><i>fill_colour</i>: Sets global default for fill_colour, see fill colour documentation for details.</li>
        <li><b>Note:</b> Chart object properties affect only objects defined after the global_config entry.</li>
    </ul>
<h2>Operations</h2>
    <ul>
        <li>Provides a way to make tranforms to chart data (e.g. filtering).</li>
        <li>Syntax</li>
        <ul>
            <li>Operations are defined with keys starting with <i>operation_</i>, for example operation_123</li>
            <li>The part after "operation_" is order tag which defines order of operations</li>
            <ul>
                <li>For example { "operation_b":"someOp(1,1)", "operation_a":"anotherOp(1,2)" } defines two operations which are run in order anotherOp, someOp</li>
                <li>Note that ordering is lexicographical (e.g. "operation_10" would be executed before "operation_9")
            </ul>
        </ul>
        <li>Available operations</li>
        <ul>
            <li><i>passWindow</i></li>
            <ul>
                <li>Filters out points whose chosen coordinate value is out of range</li>
                <li>Example: passWindow(y, 0, 10). Filters out points whose y-value is not within [0, 10]</li>
                <li>Example: passWindow(x, 2020-08-26 12:00:00, 2020-08-26 14:00:00). Filters out points whose x-value is not within [2020-08-26 12:00:00, 2020-08-26 14:00:00]</li>
                <li>Parameters</li>
                <ul>
                    <li>0: [x,y]: coordinate whose value is evaluated</li>
                    <li>1: window lower bound (inclusive).
                    <li>2: window upper bound (inclusive).
                </ul>
            </ul>
        </ul>
    </ul>
)ENDTAG");
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
        pLayout->addWidget(pTextEdit);
        removeContextHelpButtonFromDialog(m_spGuideWidget.get());
        m_spGuideWidget->resize(600, 500);
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

// Implementations for Qt Charts
#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
class XySeriesQtChart : public ::DFG_MODULE_NS(charts)::XySeries
{
public:
    XySeriesQtChart(QAbstractSeries* xySeries)
        : m_spXySeries(xySeries)
    {

    }

    void setOrAppend(const DataSourceIndex index, const double x, const double y) override
    {
        auto pXySeries = getXySeriesImpl();
        if (!pXySeries)
            return;
        if (index < static_cast<DataSourceIndex>(pXySeries->count()))
            pXySeries->replace(static_cast<int>(index), x, y);
        else
            pXySeries->append(x, y);
    }

    void resize(const DataSourceIndex nNewSize) override
    {
        auto pXySeries = getXySeriesImpl();
        if (!pXySeries || nNewSize == static_cast<DataSourceIndex>(pXySeries->count()))
            return;
        if (nNewSize < static_cast<DataSourceIndex>(pXySeries->count()))
            pXySeries->removePoints(static_cast<int>(nNewSize), pXySeries->count() - static_cast<int>(nNewSize));
        else // Case: new size is bigger than current.
        {
            const auto nAddCount = nNewSize - static_cast<DataSourceIndex>(pXySeries->count());
            for (DataSourceIndex i = 0; i < nAddCount; ++i)
                pXySeries->append(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN());
        }
    }

    QXYSeries* getXySeriesImpl()
    {
        return qobject_cast<QXYSeries*>(m_spXySeries.data());
    }

    void setValues(InputSpan<double> xVals, InputSpan<double> yVals, const std::vector<bool>* filterFlags = nullptr) override
    {
        auto pXySeries = getXySeriesImpl();
        if (!pXySeries || xVals.size() != yVals.size())
            return;
        pXySeries->clear();
        QVector<QPointF> points;
        const auto nNewCount = xVals.size();
        points.resize(static_cast<int>(nNewCount));
        std::transform(xVals.cbegin(), xVals.cbegin() + nNewCount,
                       yVals.cbegin(),
                       points.begin(),
                       [](const double x, const double y)
        {
            return QPointF(x, y);
        });
        pXySeries->replace(points);
    }

    QPointer<QAbstractSeries> m_spXySeries;
}; // Class XySeriesQtChart
#endif // #if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)

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
public:
    XySeriesQCustomPlot(QCPGraph* xySeries);

    void setOrAppend(const DataSourceIndex nIndex, const double x, const double y) override
    {
        DFG_UNUSED(nIndex);
        DFG_UNUSED(x);
        DFG_UNUSED(y);
        DFG_ASSERT_IMPLEMENTED(false);

#if 0 // Commented out for now: QCustomPlot seems to expect sorted data, would need to figure out where to do sorting, can't do while populating.
        auto spData = getXySeriesData();
        if (!spData)
            return;
        const auto nOldSize = static_cast<DataSourceIndex>(spData->size());
        if (nIndex < nOldSize)
        {
            *(spData->begin() + nIndex) = QCPGraphData(x, y);
        }
        else
        {
            spData->add(QCPGraphData(x, y));
        }
#endif
    }

    void resize(const DataSourceIndex nNewSize) override
    {
        auto spData = getXySeriesData();
        if (!spData)
            return;
        const auto nOldSize = static_cast<DataSourceIndex>(spData->size());
        if (nOldSize == nNewSize)
            return;
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
                spData->add(QCPGraphData(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()));

        }
    }

    QSharedPointer<QCPGraphDataContainer> getXySeriesData()
    {
        auto pXySeries = getXySeriesImpl();
        return (pXySeries) ? pXySeries->data() : nullptr;
    }

    QCPGraph* getXySeriesImpl()
    {
        return qobject_cast<QCPGraph*>(m_spXySeries.data());
    }

    void setValues(InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags) override
    {
        auto xySeries = getXySeriesImpl();
        if (!xySeries || xVals.size() != yVals.size())
            return;

        auto iterY = yVals.cbegin();
        QVector<QCPGraphData> data;
        if (pFilterFlags && pFilterFlags->size() == xVals.size())
        {
            auto iterFlag = pFilterFlags->begin();
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++iterFlag)
            {
                if (*iterFlag)
                    data.push_back(QCPGraphData(*iterX, *iterY));
            }
        }
        else
        {
            data.reserve(saturateCast<int>(xVals.size()));
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY)
            {
                data.push_back(QCPGraphData(*iterX, *iterY));
            }
        }
        fillQcpPlottable(*xySeries, std::move(data));
    }

    void setLineStyle(StringViewC svStyle) override;

    void setPointStyle(StringViewC svStyle) override;

    QPointer<QCPGraph> m_spXySeries;
}; // Class XySeriesQCustomPlot

XySeriesQCustomPlot::XySeriesQCustomPlot(QCPGraph* xySeries)
    : m_spXySeries(xySeries)
{
    setBaseImplementation<ChartObjectQCustomPlot>(m_spXySeries.data());
}

void XySeriesQCustomPlot::setLineStyle(StringViewC svStyle)
{
    if (!m_spXySeries)
        return;
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

    m_spXySeries->setLineStyle(style);
}

void XySeriesQCustomPlot::setPointStyle(StringViewC svStyle)
{
    if (!m_spXySeries)
        return;
    QCPScatterStyle style = QCPScatterStyle::ssNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPScatterStyle::ssCircle;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown point style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));
        
    }
      
    m_spXySeries->setScatterStyle(style);
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


#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// End of DFG_ALLOW_QCUSTOMPLOT
//
//////////////////////////////////////////////////////////////////////////////////////////////////////


#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
class ChartCanvasQtChart : public ChartCanvas, QObject
{
public:

    ChartCanvasQtChart(QWidget* pParent = nullptr)
    {
        m_spChartView.reset(new QChartView(pParent));
        m_spChartView->setRenderHint(QPainter::Antialiasing);
        m_spChartView->setChart(new QChart); // ChartView takes ownership.
    }

    QWidget* getWidget()
    {
        return m_spChartView.get();
    }

    void setTitle(StringViewUtf8 /*svPanelId*/, StringViewUtf8 svTitle) override
    {
        auto pChart = (m_spChartView) ? m_spChartView->chart() : nullptr;
        if (pChart)
            pChart->setTitle(QString::fromUtf8(svTitle.beginRaw(), static_cast<int>(svTitle.size())));
    }

    bool hasChartObjects() const override
    {
        auto pChart = (m_spChartView) ? m_spChartView->chart() : nullptr;
        return (pChart) ? !pChart->series().isEmpty() : false;
    }

    void addXySeries() override
    {
        auto pChart = (m_spChartView) ? m_spChartView->chart() : nullptr;
        pChart->addSeries(new QLineSeries(m_spChartView.get())); // chart takes ownership
        //pChart->addSeries(new QScatterSeries(m_spChartView.get())); // chart takes ownership
    }

    void setAxisForSeries(XySeries* pSeries, const double xMin, const double xMax, const double yMin, const double yMax) override
    {
        if (!pSeries || !m_spChartView)
            return;

        auto pChart = m_spChartView->chart();

        if (!pChart)
            return;

        auto qtXySeries = dynamic_cast<XySeriesQtChart*>(pSeries);

        if (!qtXySeries)
            return;

        auto pSeriesImpl = qtXySeries->getXySeriesImpl();

        if (!pSeriesImpl)
            return;

        if (pChart->axes().isEmpty())
            pChart->createDefaultAxes();

        auto xAxes = pChart->axes(Qt::Horizontal);
        auto yAxes = pChart->axes(Qt::Vertical);
        if (!xAxes.isEmpty())
            pSeriesImpl->attachAxis(xAxes.front());
        if (!yAxes.isEmpty())
            pSeriesImpl->attachAxis(yAxes.front());

        // Setting axis ranges
        {
            if (!xAxes.isEmpty() && xAxes.front())
                xAxes.front()->setRange(xMin, xMax);
            if (!yAxes.isEmpty() && yAxes.front())
                yAxes.front()->setRange(yMin, yMax);
        }
    }

    QObjectStorage<QChartView> m_spChartView;
}; // ChartCanvasQtChart
#endif // #if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)

#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

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
    ChartCanvasQCustomPlot(QWidget* pParent = nullptr)
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

          QCustomPlot* getWidget()       { return m_spChartView.get(); }
    const QCustomPlot* getWidget() const { return m_spChartView.get(); }

    void setTitle(StringViewUtf8 svPanelId, StringViewUtf8 svTitle) override;

    bool hasChartObjects() const override
    {
        return m_spChartView && (m_spChartView->graph() != nullptr);
    }

    void addXySeries() override
    {
        if (!m_spChartView)
            return;
        m_spChartView->addGraph();
    }

    void setAxisForSeries(XySeries* pSeries, const double xMin, const double xMax, const double yMin, const double yMax) override
    {
        DFG_UNUSED(pSeries);
        DFG_UNUSED(xMin);
        DFG_UNUSED(xMax);
        DFG_UNUSED(yMin);
        DFG_UNUSED(yMax);
        if (m_spChartView)
        {
            m_spChartView->rescaleAxes();
            
            // Adding margin to axes so that min/max point markers won't get clipped by axisRect.
            const auto axisRects = m_spChartView->axisRects();
            for (auto& pAr : axisRects)
            {
                if (!pAr)
                    continue;
                const auto axes = pAr->axes();
                for (auto pAxis : axes)
                {
                    if (pAxis)
                        pAxis->scaleRange(1.1);
                }
                
            }

            m_spChartView->replot();
        }
    }

    void addContextMenuEntriesForChartObjects(void* pMenu) override;

    void removeAllChartObjects() override;

    void repaintCanvas() override;

    ChartObjectHolder<XySeries> getSeriesByIndex(const XySeriesCreationParam& param) override;
    ChartObjectHolder<XySeries> getSeriesByIndex_createIfNonExistent(const XySeriesCreationParam& param) override;

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

    // Returns true if got non-null object as argument.
    static bool toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPBars* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);

private:
    template <class This_T, class Func_T>
    static void forEachAxisRectImpl(This_T& rThis, Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func) const;

    template <class Func_T> static void forEachAxis(QCPAxisRect* pAxisRect, Func_T&& func);

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
};

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
    void forEachQCustomPlotLineStyle(Func_T&& func)
    {
        func(QCPGraph::lsNone,       QT_TR_NOOP("None"));
        func(QCPGraph::lsLine,       QT_TR_NOOP("Line"));
        func(QCPGraph::lsStepLeft,   QT_TR_NOOP("Step, left-valued"));
        func(QCPGraph::lsStepRight,  QT_TR_NOOP("Step, right-valued"));
        func(QCPGraph::lsStepCenter, QT_TR_NOOP("Step middle"));
        func(QCPGraph::lsImpulse,    QT_TR_NOOP("Impulse"));
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
    template <class Style_T, class StyleSetter_T>
    static void addGraphStyleAction(QMenu& rMenu, QCPGraph& rGraph, QCustomPlot& rCustomPlot, const Style_T currentStyle, const Style_T style, const QString& sStyleName, StyleSetter_T styleSetter)
    {
        auto pAction = rMenu.addAction(sStyleName, [=, &rGraph, &rCustomPlot]() { (rGraph.*styleSetter)(style); rCustomPlot.replot(); });
        if (pAction)
        {
            pAction->setCheckable(true);
            if (currentStyle == style)
                pAction->setChecked(true);
        }
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

        auto pGraph = qobject_cast<QCPGraph*>(pPlottable);
        if (pGraph)
        {
            // Adding line style entries
            {
                addSectionEntryToMenu(pSubMenu, tr("Line Style"));
                const auto currentLineStyle = pGraph->lineStyle();
                forEachQCustomPlotLineStyle([=](const QCPGraph::LineStyle style, const char* pszStyleName)
                {
                    addGraphStyleAction(*pSubMenu, *pGraph, *m_spChartView, currentLineStyle, style, tr(pszStyleName), &QCPGraph::setLineStyle);
                });
            }

            // Adding point style entries
            {
                addSectionEntryToMenu(pSubMenu, tr("Point Style"));
                const auto currentPointStyle = pGraph->scatterStyle().shape();
                forEachQCustomPlotScatterStyle([=](const QCPScatterStyle::ScatterShape style, const char* pszStyleName)
                {
                    addGraphStyleAction(*pSubMenu, *pGraph, *m_spChartView, currentPointStyle, style, tr(pszStyleName), &QCPGraph::setScatterStyle);
                });
            }
            continue;
        }
        auto pBars = qobject_cast<QCPBars*>(pPlottable);
        if (pBars)
        {
            addSectionEntryToMenu(pSubMenu, tr("Histogram controls not implemented"));
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

auto ChartCanvasQCustomPlot::getSeriesByIndex(const XySeriesCreationParam& param) -> ChartObjectHolder<XySeries>
{
    auto p = getWidget();
    return (p && param.nIndex >= 0 && param.nIndex < p->graphCount()) ? ChartObjectHolder<XySeries>(new XySeriesQCustomPlot(m_spChartView->graph(param.nIndex))) : nullptr;
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

auto ChartCanvasQCustomPlot::getSeriesByIndex_createIfNonExistent(const XySeriesCreationParam& param) -> ChartObjectHolder<XySeries>
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

    while (param.nIndex >= p->graphCount())
    {
        // No graph exists at requested index -> creating new.
        auto pQcpGraph = p->addGraph(pXaxis, pYaxis);
        if (!pQcpGraph)
        {
            DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error: failed to create QCPGraph-object"));
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
    }
    return getSeriesByIndex(param);
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
        if (uniqueValues.size() != ticks.size()) // Does data have duplicate bar identifiers?
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

    // This could be used to set label text direction
    //plot->xAxis->setTickLabelRotation(60);

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
        else if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPBars*>(&plottable), xy, toolTipStream)) {}
    });

    QToolTip::showText(pEvent->globalPos(), toolTipStream.toPlainText());
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

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals)
{
    QVector<DataType_T> values;
    const auto nSize = saturateCast<int>(Min(xVals.size(), yVals.size()));
    values.resize(nSize);
    // Copying from [x], [y] input to [(x,y)] storage that QCPStorage uses.
    std::transform(xVals.cbegin(), xVals.cbegin() + nSize, yVals.cbegin(), values.begin(), [](const double x, const double y) { return DataType_T(x, y); });
    fillQcpPlottable(rChartObject, std::move(values));
}

#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

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
#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
    auto pChartCanvas = new ChartCanvasQtChart(this);
    auto pWidget = pChartCanvas->getWidget();
    if (pWidget)
        pLayout->addWidget(pWidget);
    m_spChartCanvas.reset(pChartCanvas);
#elif defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
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
    menu.addAction(tr("Refresh"), pParentGraphWidget, &GraphControlAndDisplayWidget::refresh);
    {
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
            DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: %2").arg(defEntry.index()).arg(viewToQString(sErrorString)));
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
                DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: unknown type '%2'").arg(defEntry.index()).arg(untypedViewToQStringAsUtf8(sEntryType)));
            }
            else
                DFG_QT_CHART_CONSOLE_WARNING(tr("Entry %1: unknown property '%2'").arg(defEntry.index()).arg(viewToQString(svId)));
        });
        if (bUnknownType)
            return;

        // Handling case type == global_config
        if (sEntryType == ChartObjectChartTypeStr_globalConfig)
        {
            globalConfigEntry = defEntry;
            pGlobalConfigEntry = &globalConfigEntry;
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
            // Checking that source type in compatible with graph type
            const auto dataType = source.dataType();
            if (dataType != GraphDataSourceType_tableSelection) // Currently only one type is supported.
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: Unknown source type").arg(defEntry.index()));
                return;
            }

            if (sEntryType == ChartObjectChartTypeStr_xy)
                refreshXy(rChart, configParamCreator, source, defEntry, nGraphCounter);
            else if (sEntryType == ChartObjectChartTypeStr_histogram)
                refreshHistogram(rChart, configParamCreator, source, defEntry, nHistogramCounter);
            else if (sEntryType == ChartObjectChartTypeStr_bars)
                refreshBars(rChart, configParamCreator, source, defEntry, nBarsCounter);
            else
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: missing handler for type '%2'").arg(defEntry.index()).arg(sEntryType.c_str()));
                return;
            }
        });
    });

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
            DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: Unknown source type, got %2").arg(defEntry.index()).arg(viewToQString(items.key())));
            return GraphDataSource::invalidIndex();
        }
        if (items.valueCount() != 1)
        {
            DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: Unexpected value count for %2: expected 1, got %3").arg(defEntry.index()).arg(ChartObjectSourceTypeStr_columnName).arg(items.valueCount()));
            return GraphDataSource::invalidIndex();
        }
        const auto svColumnName = items.value(0).toStringView();
        const auto columnIndex = dataSource.columnIndexByName(svColumnName);
        if (columnIndex == dataSource.invalidIndex())
        {
            DFG_QT_CHART_CONSOLE_WARNING(QString("Entry %1: no column '%2' found from source").arg(defEntry.index()).arg(viewToQString(svColumnName)));
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
    auto spSeries = rChart.getSeriesByIndex_createIfNonExistent(XySeriesCreationParam(nGraphCounter++, configParamCreator(), defEntry, xType, yType, qStringToStringUtf8(sXname), qStringToStringUtf8(sYname)));
    if (!spSeries)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Entry %1: couldn't create series object").arg(defEntry.index()));
        return;
    }
    auto& rSeries = *spSeries;

    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;

    using IntervalSet = ::DFG_MODULE_NS(cont)::IntervalSet<int>;
    IntervalSet xRows;
    IntervalSet* pxRowSet = nullptr;

    const auto xRowsStr = defEntry.fieldValueStr(ChartObjectFieldIdStr_xRows, []() { return StringUtf8(DFG_UTF8("*")); });
    if (!xRowsStr.empty() && xRowsStr != DFG_UTF8("*"))
    {
        xRows = ::DFG_MODULE_NS(cont)::intervalSetFromString<int>(xRowsStr.rawStorage());
        DFG_QT_CHART_CONSOLE_DEBUG(QString("Entry %1: x_rows-entry defines %2 row(s)").arg(defEntry.index()).arg(xRows.sizeOfSet()));
        pxRowSet = &xRows;
    }

    // xValueMap is also used as final (x,y) table passed to series.
    // releaseOrCopy() will return either moved data or copy of it.
    auto xValueMap = (pXdata != pYdata) ? tableData.releaseOrCopy(pXdata) : *pXdata;
    xValueMap.setSorting(false);
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
    for (const auto& kv : defEntry.m_operationMap)
    {
        auto opCopy = kv.second;
        opCopy(operationData);
    }

    // Setting values to series.
    rSeries.setValues(xValueMap.keyRange(), xValueMap.valueRange());

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

    // Rescaling axis.
    rChart.setAxisForSeries(&rSeries, minMaxX.minValue(), minMaxX.maxValue(), minMaxY.minValue(), minMaxY.maxValue());
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
        DFG_QT_CHART_CONSOLE_WARNING(QString(tr("Unrecognined bin_type '%1', using default").arg(sBinType.c_str().c_str())));
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

        auto pSingleColumn = optTableData->columnDataByIndex(columnIndexes[0]);

        if (!pSingleColumn)
            return;

        const auto valueRange = pSingleColumn->valueRange();
        if (valueRange.size() < 1)
        {
            DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: too few points (%2) for histogram").arg(defEntry.index()).arg(valueRange.size()));
            return;
        }
        const auto xType = optTableData->columnDataType(pSingleColumn);
        const auto sXaxisName = optTableData->columnName(pSingleColumn);
        spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, valueRange, xType, qStringToStringUtf8(sXaxisName)));

    }
    else // Case text valued
    {
        auto pStrings = optTableData->columnStringsByIndex(columnIndexes[0]);
        if (!pStrings || pStrings->empty())
        {
            DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: no data found for histogram").arg(defEntry.index()));
            return;
        }
        const auto sXaxisName = optTableData->columnName(columnIndexes[0]);
        spHistogram = rChart.createHistogram(HistogramCreationParam(configParamCreator(), defEntry, pStrings->valueRange(), qStringToStringUtf8(sXaxisName)));
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

    const auto firstValues = pFirstCol->valueRange();
    const auto secondValues = pSecondCol->valueRange();

    const auto sXaxisName = optTableData->columnName(columnIndexes[0]);
    const auto sYaxisName = optTableData->columnName(columnIndexes[1]);

    auto spBarSeries = rChart.createBarSeries(BarSeriesCreationParam(configParamCreator(), defEntry, firstValues, secondValues, ChartDataType::unknown,
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
