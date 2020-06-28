#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"
#include "../dfgAssert.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/valueArray.hpp"
#include "../cont/ViewableSharedPtr.hpp"
#include "../charts/commonChartTools.hpp"
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


// Abstract class representing graph data source.
class GraphDataSource : public QObject
{
    Q_OBJECT
public:
    typedef ::DFG_MODULE_NS(qt)::DataSourceIndex DataSourceIndex;
    using DoubleValueVector = DFG_MODULE_NS(cont)::ValueVector<double>;
    using SingleColumnDoubleValuesOptional = std::shared_ptr<const DoubleValueVector>;
    using ColumnDataTypeMap = ::DFG_MODULE_NS(cont)::MapVectorAoS<DataSourceIndex, ChartDataType>;
    using ColumnNameMap = ::DFG_MODULE_NS(cont)::MapVectorAoS<DataSourceIndex, QString>;
    using IndexList = ::DFG_MODULE_NS(cont)::ValueVector<DataSourceIndex>;
    using ForEachElementByColumHandler = std::function<void(const double*, const double*, const QVariant*, DataSourceIndex)>; // row array, value array (null if not available as doubles), value array as variant (null if not available), array size

    virtual ~GraphDataSource() {}

    GraphDataSourceId uniqueId() const { return m_uniqueId; }

    GraphDataSourceType dataType() const { return GraphDataSourceType_tableSelection; }

    virtual QObject* underlyingSource() = 0;

    virtual void forEachElement_byColumn(DataSourceIndex, ForEachElementByColumHandler) { DFG_ASSERT_IMPLEMENTED(false); }

    virtual SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(DataSourceIndex /*offsetFromFirst*/) { return SingleColumnDoubleValuesOptional(); }
    virtual SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(DataSourceIndex) { return SingleColumnDoubleValuesOptional(); }

    virtual ColumnDataTypeMap columnDataTypes() const { return ColumnDataTypeMap(); }
    virtual ColumnNameMap     columnNames()     const { return ColumnNameMap(); }

    virtual IndexList       columnIndexes() const                         { return IndexList(); }
    virtual DataSourceIndex columnCount()   const                         { return 0; }
    virtual DataSourceIndex columnIndexByName(const StringViewUtf8) const { return invalidIndex(); }

    // Enables or disables data source. When disabled, data source may not emit sigChanged() signals or update it's internal data structures.
    // It may, however, respond to data requests e.g. if data is readily available in it's data structures.
    virtual void enable(bool) = 0;

    // Sets definition viewer so that source can, if needed, know what data is needed from it.
    // Development note: for now as a quick solution interface is restricted to setting only one viewer.
    virtual void setChartDefinitionViewer(ChartDefinitionViewer) {}

    bool hasChangeSignaling() const { return m_bAreChangesSignaled; }

    static DataSourceIndex invalidIndex() { return NumericTraits<DataSourceIndex>::maxValue; }

    // Static helpers fro converting different types to double value.
    static double dateToDouble(QDateTime&& dt);
    static double timeToDouble(const QTime& t);
    static double stringToDouble(const QString& s);
    static double cellStringToDouble(const QString& s, const DataSourceIndex nCol, ColumnDataTypeMap& typeMap);

signals:
    void sigChanged(); // If source support signaling (see hasChangeSignaling()), emitted when data has changed.

public:
    GraphDataSourceId m_uniqueId; // Unique ID by which data source can be queried with.
    bool m_bAreChangesSignaled = false;
}; // Class GraphDataSource

class DataSourceContainer
{
public:
    QString idListAsString() const;
    size_t size() const { return m_sources.size(); }

    std::vector<std::shared_ptr<GraphDataSource>> m_sources;
}; // DataSourceContainer


// Interface for controlling charts
class ChartController : public QFrame
{
    Q_OBJECT
public slots:
    void refresh();
    void clearCaches() { clearCachesImpl(); }
    GraphDataSourceId defaultSourceId() const { return defaultSourceIdImpl(); }

private:
    virtual void refreshImpl() = 0; // Guaranteed to not get called by refresh() while already in progress.
    virtual void clearCachesImpl() {}
    virtual GraphDataSourceId defaultSourceIdImpl() const { return GraphDataSourceId(); }

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

signals:
    void sigPreferredSizeChanged(QSize);
    void sigGraphEnableCheckboxToggled(bool b);

public slots:
    void onShowControlsCheckboxToggled(bool b);
    void onShowConsoleCheckboxToggled(bool b);

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

    GraphControlAndDisplayWidget();
    ~GraphControlAndDisplayWidget() override;

    void addDataSource(std::unique_ptr<GraphDataSource> spSource);

    void setDefaultDataSourceId(const GraphDataSourceId& sDefaultDataSource);

    void forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)>);

    void privForEachDataSource(std::function<void(GraphDataSource&)> func);

public slots:
    void onDataSourceChanged();
    void onDataSourceDestroyed();
    void onControllerPreferredSizeChanged(QSize sizeHint);
    void onGraphEnableCheckboxToggled(bool b);

    // Controller implementations
private:
    void refreshImpl() override;
    void clearCachesImpl() override;
    GraphDataSourceId defaultSourceIdImpl() const override;

private:
    using ConfigParamCreator = std::function<::DFG_MODULE_NS(charts)::ChartConfigParam ()>;
    void refreshXy(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nCounter);
    void refreshHistogram(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nCounter);
    void handlePanelProperties(ChartCanvas& rChart, const GraphDefinitionEntry& defEntry);

    void setCommonChartObjectProperties(ChartObject& rObject, const GraphDefinitionEntry& defEntry, const DefaultNameCreator& defaultNameCreator);

    GraphDefinitionWidget* getDefinitionWidget();

public:
    QObjectStorage<QSplitter> m_spSplitter;
    QObjectStorage<GraphControlPanel> m_spControlPanel;
    QObjectStorage<GraphDisplay> m_spGraphDisplay;
    DataSourceContainer m_dataSources;
    GraphDataSourceId m_sDefaultDataSource;

    std::unique_ptr<ChartDataCache> m_spCache;
    bool m_bRefreshPending = false; // Set to true if refresh has been scheduled so new refresh requests can be ignored.

}; // Class GraphControlAndDisplayWidget

} } // module namespace
