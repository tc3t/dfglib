#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"
#include "../dfgAssert.hpp"
#include <memory>
#include <vector>
#include <functional>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QFrame>
    #include <QVariant>
DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

typedef QString GraphDataSourceId;
typedef std::size_t DataSourceIndex;
class ChartCanvas;

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

    virtual ~GraphDataSource() {}

    GraphDataSourceId uniqueId() const { return GraphDataSourceId(); }

    GraphDataSourceType dataType() const { return GraphDataSourceType_tableSelection; }

    virtual QObject* underlyingSource() = 0;

    virtual void forEachElement_fromTableSelection(std::function<void (DataSourceIndex, DataSourceIndex, QVariant)>) { DFG_ASSERT_IMPLEMENTED(false);  }

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


// Widget that shows graph display, no controls.
class GraphDisplay : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphDisplay(QWidget* pParent);
    ~GraphDisplay();

    ChartCanvas* chart();

    std::unique_ptr<ChartCanvas> m_spChartCanvas;
}; // Class GraphDisplay


// Widgets that shows graph controls, no actual display.
class GraphControlPanel : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphControlPanel(QWidget* pParent);
}; // Class GraphControlPanel


// Widget that shows both graph display and controls.
class GraphControlAndDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    typedef QFrame BaseClass;

    GraphControlAndDisplayWidget();

    void refresh();

    void addDataSource(std::unique_ptr<GraphDataSource> spSource);

    void forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)>);

public slots:
    void onDataSourceChanged();
    void onDataSourceDestroyed();

public:
    QObjectStorage<GraphControlPanel> m_spControlPanel;
    QObjectStorage<GraphDisplay> m_spGraphDisplay;
    DataSourceContainer m_dataSources;

}; // Class GraphControlAndDisplayWidget

} } // module namespace
