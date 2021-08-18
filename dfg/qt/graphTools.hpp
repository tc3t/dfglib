#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"
#include "../dfgAssert.hpp"
#include "../cont/MapToStringViews.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/valueArray.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../charts/commonChartTools.hpp"
#include "../OpaquePtr.hpp"
#include <memory>
#include <vector>
#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QFrame>
    #include <QVariant>
DFG_END_INCLUDE_QT_HEADERS

class QDateTime;
class QSplitter;
class QTime;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts) {
    class ChartCanvas;
}}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

typedef QString GraphDataSourceId;
typedef std::size_t DataSourceIndex;
class GraphDefinitionWidget;
class GraphDefinitionEntry;
class ChartDataCache;
class DefaultNameCreator;
using ChartCanvas = DFG_MODULE_NS(charts)::ChartCanvas;
using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

enum GraphDataSourceType
{
    GraphDataSourceType_tableSelection // Arbitrarily shaped selection from a table-like structure like table view.
};

// Returns identifier of chart backend in unspecified format, empty if not defined.
QString chartBackendImplementationIdStr();

// Represents an object that has all instructions needed to construct a chart and do operations on it.
class ChartDefinition
{
public:
    ChartDefinition(const QString& sJson = QString(), GraphDataSourceId defaultSourceId = GraphDataSourceId());

    // Calls given handler for each entry. Note that entry given to handler is temporary so it's address must not be stored and used after handler call; make a copy if it needs to be stored.
    template <class Func_T>
    void forEachEntry(Func_T&& handler) const;

    // Like forEachEntry(), but with while-condition.
    template <class While_T, class Func_T>
    void forEachEntryWhile(While_T&& whileFunc, Func_T&& handler) const;

    bool isSourceUsed(const GraphDataSourceId& sourceId, bool* pHasErrorEntries = nullptr) const;

    QStringList m_controlEntries;
    GraphDataSourceId m_defaultSourceId;
};

using ChartDefinitionViewable = ::DFG_MODULE_NS(cont)::ViewableSharedPtr<ChartDefinition>;
using ChartDefinitionViewer = ::DFG_MODULE_NS(cont)::ViewableSharedPtrViewer<ChartDefinition>;


// Provides a block of data from GraphDataSource, basically a variant of spans of different type.
class SourceDataSpan
{
public:
    template <class T> using SpanT = RangeIterator_T<const T*>;

    void clear()
    {
        std::memset(this, 0, sizeof(*this));
    }

    template <class RowIterable_T>
    void setRows(const RowIterable_T& rows)
    {
        privSetSpan(SpanT<double>(makeRange(rows)), m_pRowSpan);
    }

    // Sets values
    template <class Iterable_T>
    void set(const Iterable_T& values)
    {
        using ValueT = typename ::DFG_MODULE_NS(cont)::ElementType<Iterable_T>::type;
        privSetSpan(SpanT<ValueT>(makeRange(values)));
    }

    bool empty() const
    {
        return size() == 0;
    }

    DataSourceIndex size() const
    {
        return m_nSize;
    }

    //////////////////////////////////////////////
    // Privates

    template <class T, class DestPtr_T>
    void privSetSpan(const SpanT<T> vals, DestPtr_T& p)
    {
        p = (!vals.empty()) ? vals.beginAsPointer() : nullptr;
        DFG_ASSERT(m_nSize == 0 || vals.size() == size());
        m_nSize = (m_nSize == 0) ? vals.size() : Min(m_nSize, vals.size());
    }

    void privSetSpan(const SpanT<double> doubles)       { privSetSpan(doubles, m_pDoubleSpan); }
    void privSetSpan(const SpanT<QVariant> variants)    { privSetSpan(variants, m_pVariantSpan); }
    void privSetSpan(const SpanT<StringViewUtf8> views) { privSetSpan(views, m_pUtf8ViewSpan); }

    template <class T>
    SpanT<T> privMakeSpan(const T* pSource) const
    {
        return SpanT<T>(pSource, pSource + ((pSource) ? size() : 0));
    }

    SpanT<double>         rows()        const { return privMakeSpan(m_pRowSpan); }
    SpanT<double>         doubles()     const { return privMakeSpan(m_pDoubleSpan); }
    SpanT<StringViewUtf8> stringViews() const { return privMakeSpan(m_pUtf8ViewSpan); }

    const double*         m_pRowSpan      = nullptr;
    const double*         m_pDoubleSpan   = nullptr;
    const QVariant*       m_pVariantSpan  = nullptr;
    const StringViewUtf8* m_pUtf8ViewSpan = nullptr;
    DataSourceIndex       m_nSize = 0; // Defines the size for any non-null span.
}; // class SourceDataSpan

// Helper abstract class making it possible for data source to fill destination buffers directly instead using temporary buffers
class GraphDataSourceDataPipe
{
public:
    virtual ~GraphDataSourceDataPipe() {}
    // If ppRows/ppValues is non-null, it gets address of beginning of span of size nCount or nullptr if buffer is not availble.
    virtual void getFillBuffers(const DataSourceIndex nCount, double** ppRows, double** ppValues) = 0;
}; // class GraphDataSourceDataPipe

// Concrete GraphDataSourceDataPipe for filling MapVectorSoA<double, double, ValueVector<double>, ValueVector<double>
class GraphDataSourceDataPipe_MapVectorSoADoubleValueVector : public GraphDataSourceDataPipe
{
public:
    using ValueVectorD = ::DFG_MODULE_NS(cont)::ValueVector<double>;
    using RowToValueMap = ::DFG_MODULE_NS(cont)::MapVectorSoA<double, double, ValueVectorD, ValueVectorD>;

    GraphDataSourceDataPipe_MapVectorSoADoubleValueVector(RowToValueMap* p);

    // Note: if called with ppRows == nullptr && ppValue != nullptr, every added value has the same key.
    void getFillBuffers(const DataSourceIndex nCount, double** ppRows, double** ppValue) override;

    RowToValueMap* m_pData = nullptr;
}; // class DataPipeForTableCache

class DataQueryDetails
{
public:
    using DataMaskT = int;

    explicit DataQueryDetails(DataMaskT dataMask)
        : m_dataMask(dataMask)
    {}

    enum DataMask
    {
        DataMaskRows     = 0x1,
        DataMaskNumerics = 0x2,
        DataMaskStrings  = 0x4,
        DataMaskRowsAndNumerics = DataMaskRows | DataMaskNumerics,
        DataMaskRowsAndStrings  = DataMaskRows | DataMaskStrings,
        DataMaskAll             = ~0
    };

    bool areRowsRequested()    const { return (m_dataMask & DataMaskRows)     != 0; }
    bool areNumbersRequested() const { return (m_dataMask & DataMaskNumerics) != 0; }
    bool areStringsRequested() const { return (m_dataMask & DataMaskStrings)  != 0; }

    DataMaskT m_dataMask = DataMaskRowsAndNumerics;
};


// Abstract class representing graph data source.
class GraphDataSource : public QObject
{
    Q_OBJECT
public:
    typedef ::DFG_MODULE_NS(qt)::DataSourceIndex DataSourceIndex;
    using String = QString;
    using DoubleValueVector = DFG_MODULE_NS(cont)::ValueVector<double>;
    using SingleColumnDoubleValuesOptional = std::shared_ptr<const DoubleValueVector>;
    using ColumnDataTypeMap = ::DFG_MODULE_NS(cont)::MapVectorAoS<DataSourceIndex, ChartDataType>;
    using ColumnNameMap = ::DFG_MODULE_NS(cont)::MapVectorAoS<DataSourceIndex, String>;
    using IndexList = ::DFG_MODULE_NS(cont)::ValueVector<DataSourceIndex>;
    using ForEachElementByColumHandler = std::function<void(const SourceDataSpan&)>;

    virtual ~GraphDataSource() {}

    GraphDataSourceId uniqueId() const { return m_uniqueId; }

    GraphDataSourceType dataType() const { return GraphDataSourceType_tableSelection; }

    // Returns true iff reading can be attempted from this source. This can be false e.g. if source refers to file that is non-existent or unreadable.
    bool isAvailable() const { return m_bIsAvailable; }

    // Tries to refresh availability status and returns effective isAvailable()
    bool refreshAvailability() { refreshAvailabilityImpl(); return isAvailable(); }

    // Returns (human readable) description of source status, empty by default.
    String statusDescription() const { return statusDescriptionImpl(); }

    // Returns underlying source object if applicable.
    virtual QObject* underlyingSource() { return nullptr; }

    // Calls handler so that it receives every element in given column. The order of rows in which data is given to handler is unspecified.
    virtual void forEachElement_byColumn(DataSourceIndex, const DataQueryDetails&, ForEachElementByColumHandler) { DFG_ASSERT_IMPLEMENTED(false); }

    // Fills destination with number data from requested column through DataPipe.
    virtual void fetchColumnNumberData(GraphDataSourceDataPipe&& pipe, const DataSourceIndex nColumn, const DataQueryDetails& queryDetails);

    virtual ColumnDataTypeMap columnDataTypes() const { return ColumnDataTypeMap(); }
    virtual ChartDataType     columnDataType(DataSourceIndex) const;
    virtual ColumnNameMap     columnNames()     const { return ColumnNameMap(); }

    virtual IndexList       columnIndexes() const                         { return IndexList(); }
    virtual DataSourceIndex columnCount()   const                         { return 0; }
    virtual DataSourceIndex columnIndexByName(const StringViewUtf8) const { return invalidIndex(); }

    // Requests to enable or disable data source. When disabled, data source may not emit sigChanged() signals or update it's internal data structures.
    // It may, however, respond to data requests e.g. if data is readily available in it's data structures.
    // Function returns the value in use after this call; this may be different from requested.
    virtual bool enable(bool) = 0;

    // Sets definition viewer so that source can, if needed, know what data is needed from it.
    // Development note: for now as a quick solution interface is restricted to setting only one viewer.
    virtual void setChartDefinitionViewer(ChartDefinitionViewer) {}

    // Returns true iff 'this' emits sigChanged() when data changes or if source data can be considered non-changing.
    bool hasChangeSignaling() const { return m_bAreChangesSignaled; }

    // Returns true iff data can be safely queried from given thread.
    // Thread safety: this function is thread safe.
    bool isSafeToQueryDataFromThread(const QThread* pThread) const { return isSafeToQueryDataFromThreadImpl(pThread); }

    // Convenience function for calling isSafeToQueryDataFromThread() with current thread.
    // Thread safety: this function is thread safe.
    bool isSafeToQueryDataFromCallingThread() const;

    static DataSourceIndex invalidIndex() { return NumericTraits<DataSourceIndex>::maxValue; }

    // Static helpers fro converting different types to double value.
    static double dateToDouble(QDateTime&& dt);
    static double timeToDouble(const QTime& t);
    static double stringToDouble(const String& s);
    static double stringToDouble(const StringViewSzC& sv);
    static double cellStringToDouble(const StringViewSzUtf8& sv);
    static double cellStringToDouble(const StringViewSzUtf8& sv, const DataSourceIndex nCol, ColumnDataTypeMap* pTypeMap);

signals:
    void sigChanged(); // If source support signaling (see hasChangeSignaling()), emitted when data has changed.

protected:
    void setAvailability(const bool bAvailable) { m_bIsAvailable = bAvailable; }

private:
    virtual String statusDescriptionImpl() const { return String(); }
    virtual void refreshAvailabilityImpl() {}
    // Default implementation returns true iff given thread is the same as this->thread().
    // Note: Implementation must be thread safe.
    virtual bool isSafeToQueryDataFromThreadImpl(const QThread* pThread) const;

public:
    GraphDataSourceId m_uniqueId; // Unique ID by which data source can be queried with.
    bool m_bAreChangesSignaled = false;
    bool m_bIsAvailable = true;
}; // Class GraphDataSource

class DataSourceContainer
{
public:
    using ContainerImpl = std::vector<std::shared_ptr<GraphDataSource>>;
    using iterator = ContainerImpl::iterator;
    using const_iterator = ContainerImpl::const_iterator;

    QString idListAsString() const;
    size_t size() const { return m_sources.size(); }

    iterator end() { return m_sources.end(); }
    const_iterator end() const { return m_sources.end(); }

    iterator findById(const GraphDataSourceId& id);

    // Returns reference to data source given a dereferencable iterator.
    // Precondition: given iterator must be dereferencable.
    GraphDataSource& iterToRef(iterator iter)
    {
        DFG_ASSERT_UB(iter != end() && *iter != nullptr);
        return **iter;
    }

    ContainerImpl m_sources;
}; // DataSourceContainer


class ChartDisplayState
{
public:
    using Enum = int;
    ChartDisplayState(Enum state) :
        m_state(state)
    {}
    operator Enum() const { return m_state; }
    enum
    {
        updating,
        finalizing,
        idle
    };

private:
    int m_state = idle;
};

// Interface for controlling charts
class ChartController : public QFrame
{
    Q_OBJECT
public slots:
    void refresh();
    bool refreshTerminate(); // Returns true if terminate request was taken into consideration, false otherwise (e.g. if termination is not supported)
    void clearCaches() { clearCachesImpl(); }
public:
    GraphDataSourceId defaultSourceId() const { return defaultSourceIdImpl(); }

private:
    virtual void refreshImpl() = 0; // Guaranteed to not get called by refresh() while already in progress.
    virtual bool refreshTerminateImpl() { return false; }
    virtual void clearCachesImpl() {}
    virtual GraphDataSourceId defaultSourceIdImpl() const { return GraphDataSourceId(); }
    virtual bool setChartControls(QString) { return false; } // Returns true is given string was taken into use, false otherwise.

protected:
    bool m_bRefreshInProgress = false;
}; // Class ChartController


// Widget that shows graph display, no controls.
class GraphDisplay : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphDisplay(QWidget* pParent);
    ~GraphDisplay() override;

    void showSaveAsImageDialog();
    void clearCaches();

    ChartCanvas* chart();

    ChartController* getController();

protected:
    void contextMenuEvent(QContextMenuEvent* pEvent) override;

public:
    std::unique_ptr<ChartCanvas> m_spChartCanvas;
}; // Class GraphDisplay


// Widgets that shows graph controls, no actual display.
class GraphControlPanel : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphControlPanel(QWidget* pParent);
    ~GraphControlPanel();

    ChartController* getController();

    bool getEnabledFlag() const;
    void setEnabledFlag(bool b);

signals:
    void sigPreferredSizeChanged(QSize);
    void sigGraphEnableCheckboxToggled(bool b);

public slots:
    void onShowControlsCheckboxToggled(bool b);
    void onShowConsoleCheckboxToggled(bool b);
    void onDisplayStateChanged(ChartDisplayState state);

public:
    bool m_bLogCacheDiagnosticsOnUpdate = false;
private:
    QObjectStorage<QWidget> m_spGraphDefinitionWidget;
    QObjectStorage<QWidget> m_spConsoleWidget;
}; // Class GraphControlPanel


// Widget that shows both graph display and controls.
class GraphControlAndDisplayWidget : public ChartController
{
    Q_OBJECT

public:
    typedef ChartController BaseClass;
    using ChartObject = ::DFG_MODULE_NS(charts)::ChartObject;

    GraphControlAndDisplayWidget(bool bAllowAppSettingUsage = false);
    ~GraphControlAndDisplayWidget() override;

    void addDataSource(std::unique_ptr<GraphDataSource> spSource);

    void setDefaultDataSourceId(const GraphDataSourceId& sDefaultDataSource);

    void forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)>);

    void privForEachDataSource(std::function<void(GraphDataSource&)> func);

    // Sets chart guide. Can be set only once and must to be done during initialization.
    void setChartGuide(const QString& s);

    QString getChartDefinitionString() const;

    GraphControlPanel* getChartControlPanel() { return m_spControlPanel.get(); }

public slots:
    void onDataSourceChanged();
    void onDataSourceDestroyed();
    void onControllerPreferredSizeChanged(QSize sizeHint);
    void onGraphEnableCheckboxToggled(bool b);
    bool setChartControls(QString s) override;
    void setChartingEnabledState(bool b);

    // Controller implementations
private:
    void refreshImpl() override;
    bool refreshTerminateImpl() override;
    void clearCachesImpl() override;
    GraphDataSourceId defaultSourceIdImpl() const override;
    // setChartControls() is defined in slots-section

public:
    class ChartData;
    class ChartDataPreparator;

private:
    class RefreshContext;
    using ConfigParamCreator = std::function<::DFG_MODULE_NS(charts)::ChartConfigParam ()>;
    void refreshXy(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData);
    void refreshHistogram(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData);
    void refreshBars(RefreshContext& context, ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, ChartData* pPreparedData);
    void handlePanelProperties(ChartCanvas& rChart, const GraphDefinitionEntry& defEntry);

private:
    static ChartData prepareData(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry);
    static ChartData prepareDataForXy(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry);
    static ChartData prepareDataForHistogram(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry);
    static ChartData prepareDataForBars(std::shared_ptr<ChartDataCache>& spCache, GraphDataSource& source, const GraphDefinitionEntry& defEntry);

    void setCommonChartObjectProperties(RefreshContext& context, ChartObject& rObject, const GraphDefinitionEntry& defEntry, ConfigParamCreator configParamCreator, const DefaultNameCreator& defaultNameCreator);

    GraphDefinitionWidget* getDefinitionWidget();
    const GraphDefinitionWidget* getDefinitionWidget() const;

    ChartCanvas* chart();

public:
    class ChartRefreshParam
    {
    public:
        ChartRefreshParam();
        ChartRefreshParam(const ChartDefinition& chartDefinition, DataSourceContainer source, std::shared_ptr<ChartDataCache> spCache);
        ChartRefreshParam(const ChartRefreshParam& other);
        ~ChartRefreshParam();
        const ChartDefinition& chartDefinition() const;
        DataSourceContainer& dataSources();

        void storePreparedData(const GraphDefinitionEntry& defEntry, ChartData chartData);
        ChartData* preparedDataForEntry(const GraphDefinitionEntry& defEntry);

        void setTerminatedFlag(bool b);
        bool wasTerminated() const;

        std::shared_ptr<ChartDataCache>& cache();

        DFG_OPAQUE_PTR_DECLARE();
    }; // class ChartRefreshParam

    using ChartRefreshParamPtr = QSharedPointer<ChartRefreshParam>;

    // Starts fetching data for chart items
    void startChartDataFetching();
    // Called when chart data has been prepared and actual chart items can be created.
    void onChartDataPreparationReady(ChartRefreshParamPtr spParam);

signals:
    void sigChartDataPreparationNeeded(ChartRefreshParamPtr);

public:
    QObjectStorage<QSplitter> m_spSplitter;
    QObjectStorage<GraphControlPanel> m_spControlPanel;
    QObjectStorage<GraphDisplay> m_spGraphDisplay;
    DataSourceContainer m_dataSources;
    GraphDataSourceId m_sDefaultDataSource;

    std::shared_ptr<ChartDataCache> m_spCache;
    bool m_bRefreshPending = false; // Set to true if refresh has been scheduled so new refresh requests can be ignored.

    DFG_OPAQUE_PTR_DECLARE();

}; // Class GraphControlAndDisplayWidget


class GraphControlAndDisplayWidget::ChartDataPreparator : public QObject
{
    Q_OBJECT
public:
    void prepareData(ChartRefreshParamPtr spParam);
    void terminatePreparation(); // Request to terminate current preparation.

signals:
    void sigPreparationFinished(ChartRefreshParamPtr spParam);

private:
    DFG_OPAQUE_PTR_DECLARE();
}; // Class GraphControlAndDisplayWidget::ChartDataPreparator


namespace DFG_DETAIL_NS
{
    // Helper class that can be used for feching data from source to a temporary buffer and feeding it to data requester in 
    // batches.
    class SourceSpanBuffer
    {
    public:
        using QueryCallback = GraphDataSource::ForEachElementByColumHandler;
        using ColumnDataTypeMap = GraphDataSource::ColumnDataTypeMap;

        SourceSpanBuffer(DataSourceIndex nCol, const DataQueryDetails& queryDetails, ColumnDataTypeMap* pColumnDataTypeMap, QueryCallback func);
        ~SourceSpanBuffer();

        // Stores string data from source to temporary buffer and if buffer size get above threashold, calls submitData()
        void storeToBuffer(const DataSourceIndex nRow, const StringViewUtf8& sv);

        void storeToBuffer(const DataSourceIndex nRow, const QVariant& var);

        // Submits buffered data to QueryCallback
        void submitData();

        static constexpr size_t contentBlockSize()
        {
            return 50000; // Arbitrarily chosen
        }

        DataSourceIndex m_nColumn;
        DataQueryDetails m_queryDetails;
        ColumnDataTypeMap* m_pColumnDataTypeMap;
        QueryCallback m_queryCallback;
        ::DFG_MODULE_NS(cont)::MapToStringViews<DataSourceIndex, StringUtf8> m_stringBuffer;
        std::vector<double> m_rowBuffer;
        std::vector<double> m_valueBuffer;
        std::vector<StringViewUtf8> m_stringViewBuffer;
    }; // class SourceSpanBuffer
}

} } // module namespace
