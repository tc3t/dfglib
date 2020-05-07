#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"
#include "../dfgAssert.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/valueArray.hpp"
#include "../charts/commonChartTools.hpp"
#include <memory>
#include <vector>
#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QFrame>
    #include <QVariant>
DFG_END_INCLUDE_QT_HEADERS

class QSplitter;

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts) {
    class ChartCanvas;
}}

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

typedef QString GraphDataSourceId;
typedef std::size_t DataSourceIndex;
class GraphDefinitionEntry;
class ChartDataCache;
class DefaultNameCreator;
using ChartCanvas = DFG_MODULE_NS(charts)::ChartCanvas;
using ChartDataType = ::DFG_MODULE_NS(charts)::ChartDataType;

enum GraphDataSourceType
{
    GraphDataSourceType_tableSelection // Arbitrarily shaped selection from a table-like structure like table view.
};

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

    virtual ~GraphDataSource() {}

    GraphDataSourceId uniqueId() const { return GraphDataSourceId(); }

    GraphDataSourceType dataType() const { return GraphDataSourceType_tableSelection; }

    virtual QObject* underlyingSource() = 0;

    virtual void forEachElement_fromTableSelection(std::function<void(DataSourceIndex, DataSourceIndex, QVariant)>) { DFG_ASSERT_IMPLEMENTED(false); }

    virtual SingleColumnDoubleValuesOptional singleColumnDoubleValues_byOffsetFromFirst(DataSourceIndex /*offsetFromFirst*/) { return SingleColumnDoubleValuesOptional(); }
    virtual SingleColumnDoubleValuesOptional singleColumnDoubleValues_byColumnIndex(DataSourceIndex) { return SingleColumnDoubleValuesOptional(); }

    virtual ColumnDataTypeMap columnDataTypes() const { return ColumnDataTypeMap(); }

    virtual ColumnNameMap columnNames() const { return ColumnNameMap(); }

    virtual DataSourceIndex columnCount() const { return 0; }

    virtual DataSourceIndex columnIndexByName(const StringViewUtf8) const { return invalidIndex(); }

    // Enables or disables data source. When disabled, data source should be completely inactive, e.g. may not emit sigChanged() signals or update it's internal data structures.
    virtual void enable(bool) = 0;

    static DataSourceIndex invalidIndex() { return NumericTraits<DataSourceIndex>::maxValue; }

signals:
    void sigChanged(); // Emitted when data has changed. TODO: Add parameter?

public:
    GraphDataSourceId m_uniqueId; // Unique ID by which data source can be queried with.
}; // Class GraphDataSource

class DataSourceContainer
{
public:
    std::vector<std::shared_ptr<GraphDataSource>> m_sources;
}; // DataSourceContainer


// Interface for controlling charts
class ChartController : public QFrame
{
    Q_OBJECT
public slots:
    void refresh();

private:
    virtual void refreshImpl() = 0; // Guaranteed to not get called by refresh() while already in progress.

    bool m_bRefreshInProgress = false;
}; // Class ChartController


// Widget that shows graph display, no controls.
class GraphDisplay : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphDisplay(QWidget* pParent);
    ~GraphDisplay();

    void showSaveAsImageDialog();

    ChartCanvas* chart();

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
    ~GraphControlAndDisplayWidget();

    void addDataSource(std::unique_ptr<GraphDataSource> spSource);

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

private:
    using ConfigParamCreator = std::function<::DFG_MODULE_NS(charts)::ChartConfigParam ()>;
    void refreshXy(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nCounter);
    void refreshHistogram(ChartCanvas& rChart, ConfigParamCreator configParamCreator, GraphDataSource& source, const GraphDefinitionEntry& defEntry, int& nCounter);
    void handlePanelProperties(ChartCanvas& rChart, const GraphDefinitionEntry& defEntry);

    void setCommonChartObjectProperties(ChartObject& rObject, const GraphDefinitionEntry& defEntry, const DefaultNameCreator& defaultNameCreator);

public:
    QObjectStorage<QSplitter> m_spSplitter;
    QObjectStorage<GraphControlPanel> m_spControlPanel;
    QObjectStorage<GraphDisplay> m_spGraphDisplay;
    DataSourceContainer m_dataSources;
    std::unique_ptr<ChartDataCache> m_spCache;

}; // Class GraphControlAndDisplayWidget

} } // module namespace
