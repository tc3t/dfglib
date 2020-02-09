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

class QSplitter;

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

    // Enables or disables data source. When disabled, data source should be completely inactive, e.g. may not emit sigChanged() signals or update it's internal data structures.
    virtual void enable(bool) = 0; 

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

signals:
    void sigPreferredSizeChanged(QSize);
    void sigGraphEnableCheckboxToggled(bool b);

public slots:
    void onShowControlsCheckboxToggled(bool b);

private:
    QObjectStorage<QWidget> m_spGraphDefinitionWidget;
}; // Class GraphControlPanel


// Widget that shows both graph display and controls.
class GraphControlAndDisplayWidget : public QFrame
{
    Q_OBJECT

public:
    typedef QFrame BaseClass;

    GraphControlAndDisplayWidget();
    ~GraphControlAndDisplayWidget();

    void addDataSource(std::unique_ptr<GraphDataSource> spSource);

    void forDataSource(const GraphDataSourceId& id, std::function<void (GraphDataSource&)>);

    void privForEachDataSource(std::function<void(GraphDataSource&)> func);

public slots:
    void refresh();
    void onDataSourceChanged();
    void onDataSourceDestroyed();
    void onControllerPreferredSizeChanged(QSize sizeHint);
    void onGraphEnableCheckboxToggled(bool b);

public:
    QObjectStorage<QSplitter> m_spSplitter;
    QObjectStorage<GraphControlPanel> m_spControlPanel;
    QObjectStorage<GraphDisplay> m_spGraphDisplay;
    DataSourceContainer m_dataSources;

}; // Class GraphControlAndDisplayWidget

} } // module namespace
