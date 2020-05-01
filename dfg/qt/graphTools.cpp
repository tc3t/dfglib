#include "graphTools.hpp"

#include "connectHelper.hpp"
#include "widgetHelpers.hpp"
#include "ConsoleDisplay.hpp"
#include "ConsoleDisplay.cpp"

#include "../cont/IntervalSetSerialization.hpp"
#include "../cont/MapVector.hpp"
#include "../rangeIterator.hpp"

#include "../func/memFunc.hpp"
#include "../str/format_fmt.hpp"

#include "../alg.hpp"
#include "../math.hpp"
#include "../scopedCaller.hpp"
#include "../str/strTo.hpp"

#include "../time/timerCpu.hpp"

#include "../io/BasicImStream.hpp"
#include "../io/DelimitedTextReader.hpp"

#include "../chartsAll.hpp"

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

using namespace DFG_MODULE_NS(charts); // TODO: replace by something less coarse.

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

class ConsoleLogHandle
{
public:
    using HandlerT = std::function<void(const char*)>;

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

    void log(const char* psz)
    {
        if (m_handler)
            m_handler(psz);
    }

    void log(const QString& s)
    {
        if (m_handler)
            m_handler(s.toUtf8());
    }

    ConsoleLogLevel effectiveLevel() const { return m_effectiveLevel; }

    ConsoleLogLevel m_effectiveLevel = ConsoleLogLevel::none;
    ConsoleLogLevel m_desiredLevel = ConsoleLogLevel::info;
    HandlerT m_handler = nullptr;
};

static ConsoleLogHandle gConsoleLogHandle;

static QString viewToQString(const StringViewC& view)
{
    return QString::fromUtf8(view.data(), static_cast<int>(view.length()));
}

static QString viewToQString(const StringViewUtf8& view)
{
    return QString::fromUtf8(view.dataRaw(), static_cast<int>(view.length()));
}

} // unnamed namespace

#define DFG_QT_CHART_CONSOLE_LOG(LEVEL, MSG) if (LEVEL >= gConsoleLogHandle.effectiveLevel()) gConsoleLogHandle.log(MSG)
#define DFG_QT_CHART_CONSOLE_DEBUG(MSG)      DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::debug, MSG)
#define DFG_QT_CHART_CONSOLE_INFO(MSG)       DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::info, MSG)
#define DFG_QT_CHART_CONSOLE_WARNING(MSG)    DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::warning, MSG)
#define DFG_QT_CHART_CONSOLE_ERROR(MSG)      DFG_QT_CHART_CONSOLE_LOG(ConsoleLogLevel::error, MSG)


void ChartController::refresh()
{
    if (m_bRefreshInProgress)
        return; // refreshImpl() already in progress; not calling it.
    auto flagResetter = makeScopedCaller([&]() { m_bRefreshInProgress = true; }, [&]() { m_bRefreshInProgress = false; });
    refreshImpl();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDefinitionEntry
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: most/all of this should be in AbstractChartControlItem. For now mostly here due to the readily available json-tools in Qt.
class GraphDefinitionEntry : public AbstractChartControlItem
{
public:
    using StringViewOrOwner = StringViewOrOwner<StringViewSzC, std::string>;

    static GraphDefinitionEntry xyGraph(const QString& sColumnX, const QString& sColumnY)
    {
        QVariantMap keyVals;
        keyVals.insert(ChartObjectFieldIdStr_enabled, true);
        keyVals.insert(ChartObjectFieldIdStr_type, ChartObjectChartTypeStr_xy);
        keyVals.insert(ChartObjectFieldIdStr_lineStyle, ChartObjectLineStyleStr_basic);
        keyVals.insert(ChartObjectFieldIdStr_pointStyle, ChartObjectPointStyleStr_basic);
        DFG_UNUSED(sColumnX);
        DFG_UNUSED(sColumnY);
        GraphDefinitionEntry rv;
        rv.m_items = QJsonDocument::fromVariant(keyVals);
        return rv;
    }

    static GraphDefinitionEntry fromText(const QString& sJson, const int nIndex)
    {
        GraphDefinitionEntry rv;
        rv.m_items = QJsonDocument::fromJson(sJson.toUtf8());
        rv.m_nContainerIndex = nIndex;
        return rv;
    }

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

    GraphDataSourceId sourceId() const { return GraphDataSourceId(); }

    int index() const { return m_nContainerIndex; }

    bool isType(FieldIdStrViewInputParam fieldId) const;

private:
    QJsonValue getField(FieldIdStrViewInputParam fieldId) const; // fieldId must be a ChartObjectFieldIdStr_

    std::pair<bool, String> fieldValueStrImpl(FieldIdStrViewInputParam fieldId) const override;

    template <class Func_T>
    void doForFieldIfPresent(FieldIdStrViewInputParam id, Func_T&& func) const;


    QJsonDocument m_items;
    int m_nContainerIndex = -1;

}; // class GraphDefinitionEntry


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

bool GraphDefinitionEntry::isType(FieldIdStrViewInputParam fieldId) const
{
    return getField(ChartObjectFieldIdStr_type).toString().toUtf8().data() == fieldId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   ChartDataCache
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class TableSelectionCacheItem
{
public:
    using IndexT = GraphDataSource::DataSourceIndex;
    using RowToValueMap = ::DFG_MODULE_NS(cont)::MapVectorSoA<double, double>;
    using ColumnToValuesMap = ::DFG_MODULE_NS(cont)::MapVectorSoA<IndexT, RowToValueMap>;

    void populateFromSource(GraphDataSource& source, const GraphDefinitionEntry& defEntry);

    std::vector<std::reference_wrapper<const RowToValueMap>> columnDatas() const;

    const RowToValueMap* columnDataByIndex(IndexT nColumnIndex);

    IndexT columnCount() const;

    IndexT firstColumnIndex() const; // GraphDataSource::invalidIndex() if not defined
    IndexT lastColumnIndex() const; // GraphDataSource::invalidIndex() if not defined

    decltype(ColumnToValuesMap().keyRange()) columnRange() const;

    bool isValid() const { return m_bIsValid; }

    RowToValueMap releaseOrCopy(const RowToValueMap* pId);

    bool hasColumnIndex(const IndexT nCol) const;

    ChartDataType columnDataType(const RowToValueMap* pColumn) const;

    IndexT columnToIndex(const RowToValueMap* pColumn) const;

    ColumnToValuesMap m_colToValuesMap;
    GraphDataSource::ColumnDataTypeMap m_columnTypes;
    bool m_bIsValid = false;
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

namespace
{
    // Helper class for dealing with items of format key_name(comma-separated list of args with csv-quoting)
    // Examples: 
    //      input: 'some_id(arg one,"arg two that has , in it",arg three)' -> {key="some_id", values[3]={"arg one", "arg two that has , in it", "arg three"}
    class ParenthesisItem
    {
    public:
        using StringT = StringUtf8;
        using StringView = StringViewUtf8;

        // Given string must accessible for the lifetime of 'this'
        ParenthesisItem(const StringT& sv);
        ParenthesisItem(StringT&&) = delete;

        // Constructs from StringView promised to outlive 'this'
        static ParenthesisItem fromStableView(StringView sv);

        StringView key() const { return m_key; }

        size_t valueCount() const { return m_values.size(); }

        template <class T>
        T valueAs(const size_t nIndex) const
        {
            return ::DFG_MODULE_NS(str)::strTo<T>(value(nIndex));
        }

        StringView value(const size_t nIndex) const
        {
            return (isValidIndex(m_values, nIndex)) ? m_values[nIndex] : StringView();
        }

    private:
        ParenthesisItem(const StringView& sv);

    public:
        StringView m_key;
        std::vector<StringT> m_values;
    };

    ParenthesisItem::ParenthesisItem(const StringT& s) :
        ParenthesisItem(StringView(s))
    {
    }

    ParenthesisItem::ParenthesisItem(const StringView& sv)
    {
        auto iterOpen = std::find(sv.beginRaw(), sv.endRaw(), '(');
        if (iterOpen == sv.endRaw())
            return; // Didn't find opening parenthesis
        m_key = StringView(sv.data(), SzPtrUtf8(iterOpen));
        iterOpen++; // Skipping opening parenthesis
        auto iterEnd = sv.endRaw();
        if (iterOpen == iterEnd)
            return;
        --iterEnd;
        // Skipping trailing whitespaces
        while (iterEnd != iterOpen && DFG_MODULE_NS(alg)::contains(" ", *iterEnd))
            --iterEnd;
        // Checking that items end with closing parenthesis.
        if (*iterEnd != ')')
            return;
        // Parsing item inside parenthesis as standard comma-delimited item with quotes.
        DFG_MODULE_NS(io)::BasicImStream istrm(iterOpen, iterEnd - iterOpen);
        DFG_MODULE_NS(io)::DelimitedTextReader::read<char>(istrm, ',', '"', DFG_MODULE_NS(io)::DelimitedTextReader::s_nMetaCharNone, [&](size_t, size_t, const char* p, const size_t nSize)
        {
            m_values.push_back(StringT(TypedCharPtrUtf8R(p), TypedCharPtrUtf8R(p + nSize)));
        });
    }

    auto ParenthesisItem::fromStableView(StringView sv) -> ParenthesisItem
    {
        return ParenthesisItem(sv);
    }

} // unnamed namespace

void DFG_MODULE_NS(qt)::TableSelectionCacheItem::populateFromSource(GraphDataSource& source, const GraphDefinitionEntry& defEntry)
{
    // NOTE: any change that affects the content might require changes to generation of cacheKey, see ChartDataCache documentation for details.
    DFG_UNUSED(defEntry);
    m_colToValuesMap.clear();

    source.forEachElement_fromTableSelection([&](DataSourceIndex r, DataSourceIndex c, const QVariant& val)
    {
        auto insertRv = m_colToValuesMap.insert(std::move(c), RowToValueMap());
        if (insertRv.second) // Was new inserted?
            insertRv.first->second.setSorting(false); // Disabling sorting while adding
        insertRv.first->second.m_keyStorage.push_back(static_cast<double>(r));
        insertRv.first->second.m_valueStorage.push_back(val.toDouble());
    });

    m_columnTypes = source.columnDataTypes();

    // Sorting values by row now that all data has been added.
    for (auto iter = m_colToValuesMap.begin(), iterEnd = m_colToValuesMap.end(); iter != iterEnd; ++iter)
    {
        iter->second.setSorting(true);
    }
    m_bIsValid = true;
}

auto DFG_MODULE_NS(qt)::TableSelectionCacheItem::releaseOrCopy(const RowToValueMap* pId) -> RowToValueMap
{
    auto iter = std::find_if(m_colToValuesMap.begin(), m_colToValuesMap.end(), [=](const auto& v)
    {
        return &v.second == pId;
    });
    // For now moving always as there's no permanent caching yet.
    if (iter != m_colToValuesMap.end())
    {
        m_bIsValid = false; // Since data gets moved out, marking this cache item invalidated.
        return std::move(iter->second);
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
    return (iter != m_columnTypes.end()) ? iter->second : ChartDataType::unknown;
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
 * Current implementation in the context of dfgQtTableEditor and TableSelection source:
 *  1. User changes selection
 *  2. SelectionAnalyzerForGraphing notices changed selection and stores all data in the selection to it's own data table.
 *      Note that it does not know anything about ChartObject definitions so big proportions of the stored data might not be used at all in charts.
 *  3. Changed selection leads to recreation of ChartObjects:
 *      a. ChartObjects do not directly query data from data source, but instead they ask data from object of this class (ChartDataCache)
 *      b. ChartObject definition defines a cacheKey so that items with identical cacheKeys can use the same fecthed cache data.
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

    // Maps
    using CacheKeyToTableSelectionaMap = std::map<CacheEntryKey, TableSelectionOptional>;
    using CacheKeyToColumnDoubleValuesMap = std::map<CacheEntryKey, SingleColumnDoubleValuesOptional>;

    CacheEntryKey cacheKey(const GraphDataSource& source, const AbstractChartControlItem& defEntry) const;

    void invalidate();

    TableSelectionOptional getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry);
    SingleColumnDoubleValuesOptional getPlainColumnData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry);

    CacheKeyToTableSelectionaMap m_tableSelectionDatas;
    CacheKeyToColumnDoubleValuesMap m_doubleColumnValues;
}; // ChartDataCache

auto DFG_MODULE_NS(qt)::ChartDataCache::getTableSelectionData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> TableSelectionOptional
{
    auto key = this->cacheKey(source, defEntry);
    auto iter = m_tableSelectionDatas.find(key);
    if (iter != m_tableSelectionDatas.end() && iter->second && iter->second->isValid())
        return iter->second;

    if (iter != m_tableSelectionDatas.end())
        m_tableSelectionDatas.erase(iter); // Removing existing but invalid entry to make sure it doesn't mess up new data.

    auto rv = std::make_shared<TableSelectionCacheItem>();
    rv->populateFromSource(source, defEntry);
    m_tableSelectionDatas.insert(std::make_pair(key, rv));
    return rv;
}

auto DFG_MODULE_NS(qt)::ChartDataCache::getPlainColumnData_createIfMissing(GraphDataSource& source, const GraphDefinitionEntry& defEntry) -> SingleColumnDoubleValuesOptional
{
    auto key = this->cacheKey(source, defEntry);
    auto iter = m_doubleColumnValues.find(key);
    if (iter != m_doubleColumnValues.end() && iter->second)
        return iter->second; // Found cached, returning it.

    if (iter != m_doubleColumnValues.end())
        m_doubleColumnValues.erase(iter); // Removing existing but invalid entry.

    SingleColumnDoubleValuesOptional rv;

    const auto sXsource = defEntry.fieldValueStr(ChartObjectFieldIdStr_xSource, [] { return StringUtf8(); });
    if (sXsource.empty())
        rv = source.singleColumnDoubleValues_byOffsetFromFirst(0);
    else // Case: have non-empty x_source
    {
        const ParenthesisItem items(sXsource);
        if (items.key() == SzPtrUtf8(ChartObjectSourceTypeStr_columnName) && items.valueCount() >= 1)
        {
            const auto svColumnName = items.value(0);
            const auto nIndex = source.columnIndexByName(svColumnName);
            if (nIndex != GraphDataSource::invalidIndex())
                rv = source.singleColumnDoubleValues_byColumnIndex(nIndex);
            else
                DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: no column '%2' found from source").arg(defEntry.index()).arg(viewToQString(svColumnName)));
        }
        else
            DFG_QT_CHART_CONSOLE_WARNING(QString("Entry %1: Bad %2 specifier '%3'").arg(defEntry.index()).arg(ChartObjectFieldIdStr_xSource).arg(sXsource.rawStorage().c_str()));
    }
    m_doubleColumnValues.insert(std::make_pair(std::move(key), rv));
    return rv;
}

void DFG_MODULE_NS(qt)::ChartDataCache::invalidate()
{
    m_tableSelectionDatas.clear();
    m_doubleColumnValues.clear();
}

auto ChartDataCache::cacheKey(const GraphDataSource& source, const AbstractChartControlItem& defEntry) const -> CacheEntryKey
{
    const auto sType = defEntry.fieldValueStr(ChartObjectFieldIdStr_type, [] { return StringUtf8(); }).rawStorage();
    if (sType == ChartObjectChartTypeStr_xy)
        return source.uniqueId() + sType.c_str(); // For xySeries defEntry does not affect cache content so "source + xy" is good enough
    if (sType == ChartObjectChartTypeStr_histogram)
        return source.uniqueId() + sType.c_str() + defEntry.fieldValueStr(ChartObjectFieldIdStr_xSource, [] { return StringUtf8(); }).rawStorage().c_str(); // For histograms only the requested column is cached so adding x_source to "source + histogram"
    return "";
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

    template <class Func_T>
    void forEachDefinitionEntry(Func_T handler);

    ChartController* getController();

    QObjectStorage<QPlainTextEdit> m_spRawTextDefinition; // Guaranteed to be non-null between constructor and destructor.
    QObjectStorage<QWidget> m_spGuideWidget;
    QPointer<GraphControlPanel> m_spParent;
}; // Class GraphDefinitionWidget


GraphDefinitionWidget::GraphDefinitionWidget(GraphControlPanel *pNonNullParent) :
    BaseClass(pNonNullParent),
    m_spParent(pNonNullParent)
{
    DFG_ASSERT(m_spParent.data() != nullptr);

    auto spLayout = std::make_unique<QVBoxLayout>(this);

    m_spRawTextDefinition.reset(new QPlainTextEdit(this));

    // TODO: this should be a "auto"-entry.
    m_spRawTextDefinition->setPlainText(GraphDefinitionEntry::xyGraph(QString(), QString()).toText());

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
}


QString GraphDefinitionWidget::getGuideString()
{
    return tr(R"ENDTAG(<h2>Basic examples:</h2>
<ul>
    <li>Basic graph: {"type":"xy"}
    <li>Basic graph with lines and points: {"line_style":"basic","point_style":"basic","type":"xy"}
    <li>Basic graph with all options present: {"enabled":true,"line_style":"basic","point_style":"basic","type":"xy","name":"Example graph","x_source":"column_name(column 1)", "y_source":"column_name(column 3)", "x_rows":"1:3; 5; 7:8", "panel_id":"grid(2,2)"}
    <li>Basic histogram: {"type":"histogram","name":"Basic histogram"}
    <li>Setting panel title: {"type":"panel_properties","panel_id":"grid(1,1)","title":"Title for\npanel (1,1)"}
</ul>

<h2>Common fields</h2>
<ul>
   <li>enabled: {<b>true</b>, false}. Defines whether entry is enabled, default value is true.</li>
   <li>type: Defines entry type</li>
    <ul>
        <li>xy       : Graph of (x, y) points shown sorted by x-value. When only one column is available, uses line numbers as x-values</li>
        <li>histogram: Histogram</li>
    </ul>
   <li>name: name of the object, shown e.g. in legend.</li>
   <li>panel_id: Target panel (e.g. panel where graph is drawn). Currently grid paneling is supported; syntax is "panel_id":"grid(row number, column number)". (1,1) means top left.</li>
</ul>

<h2>Fields for type <i>xy</i></h2>
    <ul>
        <li>x_source:</li>
            <ul>
                <li>column_name(name): x-values will be taken from column that has name <i>name</i>.</li>
                <li>row_index: values will be taken from row index of y-values</li>
            </ul>
        <li>y_source:</li>
            <ul>
                <li>column_name(name): y-values will be taken from column that has name <i>name</i>.</li>
                <li>row_index: values will be taken from row index of x-values</li>
            </ul>
        <li>x_rows:</li>
            <ul>
                <li>x_rows: List of rows to include as semicolon separated list, e.g. "1:3; 4; 7:8" means 1, 2, 3, 4, 7, 8.</li>
                <li>Note that in case of filtered tables, indexes refer to visible row, not the row ID shown in the row header</li>
            </ul>
        <li>line_style:</li>
            <ul>
                <li>none:        No line indicator</li>
                <li><b>basic</b>:       Straight line between data points. Default value if field not present</li>
                <li>step_left:   Horizontal line between points, y-value from left point.</li>
                <li>step_right:  Horizontal line between points, y-value from right point.</li>
                <li>step_middle: Horizontal line around point, steps at midway on both sides.</li>
                <li>pole:        Vertical line from (x, 0) -> (x, y) for each point.</li>
            </ul>
        <li>point_style:</li>
            <ul>
                <li><b>none</b> : No point indicator. Default value if field not present</li>
                <li>basic: Basic indicator for every point</li>
            </ul>
    </ul>
<h2>Fields for type <i>histogram</i></h2>
    <ul>
        <li>bin_count: Number of bins in histogram. (default is currently 100, but this may change so it is not to be relied on)</li>
        <li>x_source: Defines column from which histogram is created, usage like described in xy-type. If omitted, uses first column.
    </ul>
<h2>Fields for type <i>panel_properties</i></h2>
    <ul>
        <li>title: Panel title. New lines can be added with \n</li>
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
        pTextEdit->setHtml(getGuideString());
        pTextEdit->setReadOnly(true);
        pLayout->addWidget(pTextEdit);
        removeContextHelpButtonFromDialog(m_spGuideWidget.get());
        m_spGuideWidget->resize(600, 500);
    }
    m_spGuideWidget->show();
}

QString GraphDefinitionWidget::getRawTextDefinition() const
{
    return  (m_spRawTextDefinition) ? m_spRawTextDefinition->toPlainText() : QString();
}

template <class Func_T>
void GraphDefinitionWidget::forEachDefinitionEntry(Func_T handler)
{
    const auto sText = m_spRawTextDefinition->toPlainText();
    const auto parts = sText.splitRef('\n');
    int i = 0;
    for (const auto& part : parts)
    {
        handler(GraphDefinitionEntry::fromText(part.toString(), i++));
    }
}

ChartController* GraphDefinitionWidget::getController()
{
    return (m_spParent) ? m_spParent->getController() : nullptr;
}

// Implementations for Qt Charts
#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
class XySeriesQtChart : public XySeries
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

// Defines custom implementation for ChartObject. This is used to avoid repeating virtual overrides in ChartObjects.
class ChartObjectQCustomPlot : public ChartObject
{
public:
    ChartObjectQCustomPlot(QCPAbstractPlottable* pPlottable)
        : m_spPlottable(pPlottable)
    {}
    virtual ~ChartObjectQCustomPlot() {}

private:
    void setNameImpl(const ChartObjectStringView s) const override
    {
        if (m_spPlottable)
            m_spPlottable->setName(QString::fromUtf8(s.dataRaw(), static_cast<int>(s.size())));
    }

    QPointer<QCPAbstractPlottable> m_spPlottable;
};

class XySeriesQCustomPlot : public XySeries
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

    void setValues(InputSpan<double> xVals, InputSpan<double> yVals, const std::vector<bool>* pFilterFlags) override
    {
        auto xySeries = getXySeriesImpl();
        if (!xySeries || xVals.size() != yVals.size())
            return;

        auto iterY = yVals.cbegin();
        QVector<double> xTemp;
        QVector<double> yTemp;
        xTemp.reserve(static_cast<int>(xVals.size()));
        yTemp.reserve(static_cast<int>(xVals.size()));
        if (pFilterFlags && pFilterFlags->size() == xVals.size())
        {
            auto iterFlag = pFilterFlags->begin();
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++iterFlag)
            {
                if (*iterFlag)
                {
                    xTemp.push_back(*iterX);
                    yTemp.push_back(*iterY);
                }
            }
        }
        else
        {
            for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY)
            {
                xTemp.push_back(*iterX);
                yTemp.push_back(*iterY);
            }
        }
        xySeries->setData(xTemp, yTemp);
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
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown line style '%1', using style 'none'").arg(viewToQString(svStyle)));
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
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown point style '%1', using style 'none'").arg(viewToQString(svStyle)));
        
    }
      
    m_spXySeries->setScatterStyle(style);
}


class HistogramQCustomPlot : public Histogram
{
public:
    HistogramQCustomPlot(QCPBars* pBars);
    ~HistogramQCustomPlot();

    void setValues(InputSpan<double> xVals, InputSpan<double> yVals) override;

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

void HistogramQCustomPlot::setValues(InputSpan<double> xVals, InputSpan<double> yVals)
{
    if (!m_spBars)
        return;
    if (xVals.size() >= 3)
        m_spBars->setWidth(1.0 * (xVals[2] - xVals[1])); // TODO: bin width to be user controllable. Factor 1.0 means that there's no space between bars
    m_spBars->setWidthType(QCPBars::wtPlotCoords);
    // TODO: this is vexing: QCPBars insists of having QVector's so must allocate new buffers.
    QVector<double> x(static_cast<int>(xVals.size()));
    QVector<double> y(static_cast<int>(yVals.size()));
    std::copy(xVals.cbegin(), xVals.cend(), x.begin());
    std::copy(yVals.cbegin(), yVals.cend(), y.begin());
    m_spBars->setData(x, y);
    m_spBars->rescaleAxes(); // TODO: revise, probably not desirable when there are more than one plottable in canvas.
}


#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)


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
class ChartCanvasQCustomPlot : public DFG_MODULE_NS(charts)::ChartCanvas, QObject
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

    bool isLegendSupported() const override { return true; }
    bool isLegendEnabled() const override;
    bool enableLegend(bool) override;
    void createLegends() override;

    // Implementation details
    QCPAxisRect* getAxisRect(const ChartObjectCreationParam& param);
    QCPAxis* getAxis(const ChartObjectCreationParam& param, QCPAxis::AxisType axisType);
    QCPAxis* getXAxis(const ChartObjectCreationParam& param);
    QCPAxis* getYAxis(const ChartObjectCreationParam& param);
    bool getGridPos(const StringViewUtf8 svPanelId, int& nRow, int& nCol);

    QObjectStorage<QCustomPlot> m_spChartView;
    bool m_bLegendEnabled = false;
    QVector<QCPLegend*> m_legends; // All but the default legend
}; // ChartCanvasQCustomPlot


class ChartPanel : public QCPLayoutGrid
{
    //Q_OBJECT
public:
    ChartPanel(QCustomPlot* pQcp);
    QCPAxisRect* axisRect();

    void setTitle(StringViewUtf8 svTitle);

    QCustomPlot* m_pQcp = nullptr;
};

ChartPanel::ChartPanel(QCustomPlot* pQcp)
    : m_pQcp(pQcp)
{
}

auto ChartPanel::axisRect() -> QCPAxisRect*
{
    const auto nElemCount = this->elementCount();
    if (nElemCount == 0 || (nElemCount == 1 && qobject_cast<QCPTextElement*>(this->elementAt(0)) != nullptr))
        addElement(nElemCount, 0, new QCPAxisRect(m_pQcp));
    return qobject_cast<QCPAxisRect*>(elementAt(elementCount() - 1));
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

    // Removing legends (would get deleted automatically with AxisRect's, but doing it here expclitly to keep m_legends up-to-date.)
    {
        qDeleteAll(m_legends);
        m_legends.clear();
    }

    // Removing all but first panel
    auto pPlotLayout = p->plotLayout();
    if (pPlotLayout)
    {
        while (pPlotLayout->elementCount() > 1 && pPlotLayout->removeAt(pPlotLayout->elementCount() - 1))
        {
        }
        auto pFirstPanel = (pPlotLayout->elementCount() >= 1) ? dynamic_cast<ChartPanel*>(pPlotLayout->elementAt(0)) : nullptr;
        if (pFirstPanel)
            pFirstPanel->setTitle(StringViewUtf8());
        pPlotLayout->simplify(); // This removes the empty space that removed items free.
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
        if (type == ChartDataType::dateAndTimeMillisecond)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd hh:mm:ss.zzz"));
        else if (type == ChartDataType::dateAndTime)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd hh:mm:ss"));
        else if (type == ChartDataType::dateOnly)
            createDateTimeTicker(rAxis, QLatin1String("yyyy-MM-dd"));
        else if (type == ChartDataType::dayTimeMillisecond)
            createDateTimeTicker(rAxis, QLatin1String("hh:mm:ss.zzz"));
        else if (type == ChartDataType::dayTime)
            createDateTimeTicker(rAxis, QLatin1String("hh:mm:ss"));
        else
            rAxis.setTicker(QSharedPointer<QCPAxisTicker>(new QCPAxisTicker)); // Resets to default ticker. TODO: do only if needed (i.e. if current ticker is something else than plain QCPAxisTicker)
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
        p->addGraph(pXaxis, pYaxis);
        setAxisTicker(*pXaxis, param.xType);
        setAxisTicker(*pYaxis, param.yType);
    }
    return getSeriesByIndex(param);
}

auto ChartCanvasQCustomPlot::createHistogram(const HistogramCreationParam& param) -> ChartObjectHolder<Histogram>
{
    auto p = getWidget();
    if (!p)
        return nullptr;

    const auto valueRange = param.valueRange;

    auto minMaxPair = std::minmax_element(valueRange.cbegin(), valueRange.cend());
    if (*minMaxPair.first >= *minMaxPair.second || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.first) || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.second))
        return nullptr;

    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;

    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create histogram, no suitable target panel found'"));
        return nullptr;
    }

    auto spHistogram = std::make_shared<HistogramQCustomPlot>(new QCPBars(pXaxis, pYaxis)); // Note: QCPBars is owned by QCustomPlot-object.

    const auto nBinCount = param.definitionEntry().fieldValue<unsigned int>(ChartObjectFieldIdStr_binCount, 100);

    try
    {
        auto hist = boost::histogram::make_histogram(boost::histogram::axis::regular<>(nBinCount, *minMaxPair.first, *minMaxPair.second, "x"));
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

    return spHistogram;
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
    {
        createLegends();
    }
    else
    {
        qDeleteAll(m_legends);
        m_legends.clear();
        auto p = getWidget();
        if (p && p->legend)
            p->legend->setVisible(false);
    }
    repaintCanvas();
    return m_bLegendEnabled;
}

void ChartCanvasQCustomPlot::createLegends()
{
    // Creating legends for all AxisRects
    auto p = getWidget();
    if (!p)
        return;

    // Clearing old legends
    {
        if (p->legend)
            p->legend->clearItems();
        p->legend->clearItems();

        qDeleteAll(m_legends);
        m_legends.clear();
    }
    
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
        if (pAxisRect != p->axisRect())
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
    auto panelId = ParenthesisItem::fromStableView(svPanelId);
    if (!svPanelId.empty() && (panelId.key() != DFG_UTF8("grid") || panelId.valueCount() > 2))
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Failed to retrieve grid panel: invalid panel definition '%1'").arg(QString::fromUtf8(svPanelId.dataRaw(), static_cast<int>(svPanelId.length()))));
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

auto ChartCanvasQCustomPlot::getAxisRect(const ChartObjectCreationParam& param) -> QCPAxisRect*
{
    int nRow = 0;
    int nCol = 0;
    if (!getGridPos(param.sPanelId, nRow, nCol))
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
    auto pExistingAxisRect = qobject_cast<QCPAxisRect*>(pLayout->element(nRow, nCol));
    ChartPanel* pChartPanel = nullptr;
    if (!pExistingAxisRect)
    {
        pChartPanel = dynamic_cast<ChartPanel*>(pLayout->element(nRow, nCol));
        if (pChartPanel)
            pExistingAxisRect = pChartPanel->axisRect();
    }
    if (!pExistingAxisRect)
    {
        if (!pChartPanel)
        {
            pChartPanel = new ChartPanel(p);
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
        pPanel = new ChartPanel(p);
        if (pElement)
            pPanel->addElement(pElement); // This transfers existing element to new layout.
        pMainLayout->addElement(nRow, nCol, pPanel);
    }
    if (pPanel)
        pPanel->setTitle(svTitle);
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

    {
        auto pConsole = new ConsoleDisplay(this);
        gConsoleLogHandle.setHandler([=](const QString& s) { pConsole->addEntry(s); } );
     
        m_spConsoleWidget.reset(pConsole);
    }

    // Adding some controls
    {
        std::unique_ptr<QHBoxLayout> pFirstRowLayout(new QHBoxLayout);

        // enable / disabled control
        {
            auto pEnableCheckBox = new QCheckBox(tr("Enable"), this); // Parent owned.
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
    return qobject_cast<ChartController*>(parent());
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

        template <class T> T propertyValueAs(const PropertyMap&, const char*);
        template <> bool    propertyValueAs<bool>(const PropertyMap& obj, const char* psz)    { return obj.value(psz).toVariant().toBool(); }
        template <> int     propertyValueAs<int>(const PropertyMap& obj, const char* psz)     { return obj.value(psz).toVariant().toInt(); }
        template <> double  propertyValueAs<double>(const PropertyMap& obj, const char* psz)  { return obj.value(psz).toVariant().toDouble(); }
        template <> QString propertyValueAs<QString>(const PropertyMap& obj, const char* psz) { return obj.value(psz).toVariant().toString(); }

    }; // class GraphDisplayImageExportDialog

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

#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
        // Export actions
        {
            menu.addAction(tr("Save as image..."), this, &GraphDisplay::showSaveAsImageDialog);
        }
#endif // QCustomPlot
    }

    addSectionEntryToMenu(&menu, tr("Chart objects"));
    m_spChartCanvas->addContextMenuEntriesForChartObjects(&menu);

    menu.exec(QCursor::pos());
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

class DefaultNameCreator
{
public:
    // pszType must point to valid string for the lifetime of 'this'.
    DefaultNameCreator(const char* pszType, const int nIndex)
        : m_pszType(pszType)
        , m_nIndex(nIndex)
    {
    }

    DFG_ROOT_NS::StringUtf8 operator()() const
    {
        return DFG_ROOT_NS::StringUtf8::fromRawString(DFG_ROOT_NS::format_fmt("{} {}", m_pszType, m_nIndex));
    }

    const char* m_pszType;
    const int m_nIndex;
};

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshImpl()
{
    DFG_MODULE_NS(time)::TimerCpu timer;

    if (m_spCache)
        m_spCache->invalidate();

    // TODO: graph filling should be done in worker thread to avoid GUI freezing.

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

    auto pDefWidget = this->m_spControlPanel->findChild<GraphDefinitionWidget*>();
    if (!pDefWidget)
    {
        DFG_QT_CHART_CONSOLE_ERROR("Internal error: missing control widget");
        return;
    }

    int nGraphCounter = 0;
    int nHistogramCounter = 0;
    // Going through every item in definition entry table and redrawing them.
    pDefWidget->forEachDefinitionEntry([&](const GraphDefinitionEntry& defEntry)
    {
        if (!defEntry.isEnabled())
            return;

        const auto sEntryType = defEntry.graphTypeStr();

        // type == panel_properties
        if (sEntryType == ChartObjectChartTypeStr_panelProperties)
        {
            const auto sPanelId = defEntry.fieldValueStr(ChartObjectFieldIdStr_panelId, [] { return StringUtf8(); });
            pChart->setTitle(sPanelId, defEntry.fieldValueStr(ChartObjectFieldIdStr_title, [] { return StringUtf8(); }));
            return;
        }

        if (sEntryType != ChartObjectChartTypeStr_xy && sEntryType != ChartObjectChartTypeStr_histogram)
        {
            // Unsupported graphType.
            DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: unknown type '%2'").arg(defEntry.index()).arg(sEntryType.c_str()));
            return;
        }

        this->forDataSource(defEntry.sourceId(), [&](GraphDataSource& source)
        {
            // Checking that source type in compatible with graph type
            const auto dataType = source.dataType();
            if (dataType != GraphDataSourceType_tableSelection) // Currently only one type is supported.
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: Unknown source type").arg(defEntry.index()));
                return;
            }

            if (sEntryType == ChartObjectChartTypeStr_xy)
                refreshXy(rChart, source, defEntry, nGraphCounter);
            else if (sEntryType == ChartObjectChartTypeStr_histogram)
                refreshHistogram(rChart, source, defEntry, nHistogramCounter);
            else
            {
                DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: missing handler for type '%2'").arg(defEntry.index()).arg(sEntryType.c_str()));
                return;
            }
        });
    });

    if (pChart->isLegendEnabled())
        pChart->createLegends();

    DFG_MODULE_NS(time)::TimerCpu timerRepaint;
    rChart.repaintCanvas();
    const auto elapsedRepaint = timerRepaint.elapsedWallSeconds();
    const auto elapsed = timer.elapsedWallSeconds();
    DFG_QT_CHART_CONSOLE_INFO(tr("Refresh lasted %1 ms (repaint took %2 %)").arg(round<int>(1000 * elapsed)).arg(round<int>(100.0 * elapsedRepaint / elapsed)));
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
    auto getChosenColumnIndex(const TableSelectionCacheItem& cacheData,
                              GraphDataSource& dataSource,
                              const decltype(ChartObjectFieldIdStr_xSource)& id,
                              const GraphDefinitionEntry& defEntry,
                              const GraphDataSource::DataSourceIndex defaultValue) -> GraphDataSource::DataSourceIndex
    {
        const auto sSource = defEntry.fieldValueStr(id, [] { return StringUtf8(); });
        if (sSource.empty())
            return defaultValue;
        if (sSource == SzPtrUtf8(ChartObjectSourceTypeStr_rowIndex))
            return defaultValue + 1;
        const ParenthesisItem items(sSource);
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
        const auto sColumnName = items.value(0);
        const auto columnIndex = dataSource.columnIndexByName(sColumnName);
        if (columnIndex == dataSource.invalidIndex() || !cacheData.hasColumnIndex(columnIndex))
        {
            DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: no column '%2' found from source").arg(defEntry.index()).arg(viewToQString(sColumnName)));
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

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshXy(ChartCanvas& rChart, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nGraphCounter)
{
    if (!m_spCache)
        m_spCache.reset(new ChartDataCache);

    auto optData = m_spCache->getTableSelectionData_createIfMissing(source, defEntry);

    if (!optData)
        return;
    
    auto& tableData = *optData;

    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
    DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;
    DataSourceIndex nGraphSize = 0;

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

    decltype(rChart.getSeriesByIndex(XySeriesCreationParam(0, defEntry, ChartDataType::unknown, ChartDataType::unknown))) spSeries;

    if (tableData.columnCount() >= 1)
    {
        const auto columns = tableData.columnRange();
        const auto nLastColumnIndex = tableData.lastColumnIndex();
        const auto nDefaultValue = nLastColumnIndex + 1;
        DataSourceIndex xColumnIndex = getChosenColumnIndex(tableData, source, ChartObjectFieldIdStr_xSource, defEntry, nDefaultValue);
        DataSourceIndex yColumnIndex = getChosenColumnIndex(tableData, source, ChartObjectFieldIdStr_ySource, defEntry, nDefaultValue);

        const auto isRowIndexSpecifier = [=](const DataSourceIndex i) { return i == nLastColumnIndex + 2; };

        if (xColumnIndex == GraphDataSource::invalidIndex() || yColumnIndex == GraphDataSource::invalidIndex())
            return; // Either column was defined but not found.

        bool bXisRowIndex = isRowIndexSpecifier(xColumnIndex);
        bool bYisRowIndex = isRowIndexSpecifier(yColumnIndex);

        if (bXisRowIndex && bYisRowIndex)
        {
            DFG_QT_CHART_CONSOLE_INFO(QString("Entry %1: Both columns are specified as row_index").arg(defEntry.index()));
            return;
        }

        if (columns.size() == 1 && !bXisRowIndex && !bYisRowIndex)
        {
            if (xColumnIndex == nDefaultValue)
                bXisRowIndex = true;
            else if (yColumnIndex == nDefaultValue)
                bYisRowIndex = true;
        }
       
        makeEffectiveColumnIndexes(xColumnIndex, yColumnIndex, columns, isRowIndexSpecifier);

        auto pXdata = tableData.columnDataByIndex(xColumnIndex);
        auto pYdata = (pXdata) ? tableData.columnDataByIndex(yColumnIndex) : nullptr;

        if (!pXdata || !pYdata)
            return;

        const auto xType = (!bXisRowIndex) ? tableData.columnDataType(pXdata) : ChartDataType::unknown;
        const auto yType = (!bYisRowIndex) ? tableData.columnDataType(pYdata) : ChartDataType::unknown;
        spSeries = rChart.getSeriesByIndex_createIfNonExistent(XySeriesCreationParam(nGraphCounter++, defEntry, xType, yType));
        if (!spSeries)
        {
            DFG_QT_CHART_CONSOLE_WARNING(tr("Entry %1: couldn't create series object").arg(defEntry.index()));
            return;
        }


        // xValueMap is also used as final (x,y) table passed to series.
        // releaseOrCopy() will return either moved data or copy of it.
        auto xValueMap = (pXdata != pYdata) ? tableData.releaseOrCopy(pXdata) : *pXdata;
        xValueMap.setSorting(false);
        const auto& yValueMap = *pYdata;
        auto xIter = xValueMap.cbegin();
        auto yIter = yValueMap.cbegin();
        DataSourceIndex nActualSize = 0;
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

                xValueMap.m_keyStorage[nActualSize] = x;
                xValueMap.m_valueStorage[nActualSize] = y;
                minMaxX(x);
                minMaxY(y);
                nActualSize++;
            }
            ++xIter;
            ++yIter;
        }
        const ptrdiff_t nSizeAsPtrdiff = static_cast<ptrdiff_t>(nActualSize);
        spSeries->setValues(headRange(xValueMap.keyRange(), nSizeAsPtrdiff), headRange(xValueMap.valueRange(), nSizeAsPtrdiff));
        nGraphSize = nActualSize;
    }

    if (spSeries)
    {
        spSeries->resize(nGraphSize); // Removing excess points (if any)

        // Setting line style
        spSeries->setLineStyle(ChartObjectLineStyleStr_basic); // Default value
        defEntry.doForLineStyleIfPresent([&](const char* psz) { spSeries->setLineStyle(psz); });

        // Setting point style
        spSeries->setPointStyle(ChartObjectPointStyleStr_none); // Default value
        defEntry.doForPointStyleIfPresent([&](const char* psz) { spSeries->setPointStyle(psz); });

        // Setting object name (used e.g. in legend)
        spSeries->setName(defEntry.fieldValueStr(ChartObjectFieldIdStr_name, DefaultNameCreator("Graph", nGraphCounter)));

        // Rescaling axis.
        rChart.setAxisForSeries(spSeries.get(), minMaxX.minValue(), minMaxX.maxValue(), minMaxY.minValue(), minMaxY.maxValue());
    }
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refreshHistogram(ChartCanvas& rChart, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nHistogramCounter)
{
    const auto nColumnCount = source.columnCount();
    if (nColumnCount < 1)
        return;

    auto optValues = m_spCache->getPlainColumnData_createIfMissing(source, defEntry);

    if (!optValues)
        return;

    const auto& values = *optValues;
    if (values.size() < 2)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Entry %1: too few points (%2) for histogram").arg(defEntry.index()).arg(values.size()));
        return;
    }

    auto spHistogram = rChart.createHistogram(HistogramCreationParam(defEntry, makeRange(values)));
    if (!spHistogram)
        return;
    ++nHistogramCounter;
    spHistogram->setName(defEntry.fieldValueStr(ChartObjectFieldIdStr_name, DefaultNameCreator("Histogram", nHistogramCounter)));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::addDataSource(std::unique_ptr<GraphDataSource> spSource)
{
    if (!spSource)
        return;

    DFG_QT_VERIFY_CONNECT(connect(spSource.get(), &GraphDataSource::sigChanged, this, &GraphControlAndDisplayWidget::onDataSourceChanged));
    DFG_QT_VERIFY_CONNECT(connect(spSource.get(), &QObject::destroyed, this, &GraphControlAndDisplayWidget::onDataSourceDestroyed));

    m_dataSources.m_sources.push_back(std::shared_ptr<GraphDataSource>(spSource.release()));
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::onDataSourceChanged()
{
    refresh();
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
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::privForEachDataSource(std::function<void(GraphDataSource&)> func)
{
    DFG_MODULE_NS(alg)::forEachFwd(m_dataSources.m_sources, [&](const std::shared_ptr<GraphDataSource>& sp)
    {
        if (sp)
            func(*sp);
    });
}
