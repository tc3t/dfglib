#include "graphTools.hpp"

#include "connectHelper.hpp"

#include "../cont/MapVector.hpp"
#include "../rangeIterator.hpp"

#include "../func/memFunc.hpp"
#include "../str/format_fmt.hpp"

#include "../alg.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QPlainTextEdit>
    #include <QGridLayout>
    #include <QComboBox>
    #include <QLabel>
    #include <QSplitter>

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
#include <QVariantMap>

DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt)
{

// Defines single graph definition entry, e.g. a definition to display xy-line graph from input from table T with x values from column A in selection and y values from column B.
class GraphDefinitionEntry
{
public:
    static GraphDefinitionEntry xyGraph(const QString& sColumnX, const QString& sColumnY)
    {
        QVariantMap keyVals;
        keyVals.insert("enabled", true);
        keyVals.insert("x_source", sColumnX);
        keyVals.insert("y_source", sColumnY);
        GraphDefinitionEntry rv;
        rv.m_items = QJsonDocument::fromVariant(keyVals);
        return rv;
    }

    static GraphDefinitionEntry fromJson(const QString& sJson)
    {
        GraphDefinitionEntry rv;
        rv.m_items = QJsonDocument::fromJson(sJson.toUtf8());
        return rv;
    }

    GraphDefinitionEntry() {}

    QString toJson() const
    {
        return m_items.toJson(QJsonDocument::Compact);
    }

    GraphDataSourceId sourceId() const { return GraphDataSourceId(); }

    QJsonDocument m_items;
}; // class GraphDefinitionEntry

class GraphDefinitionWidget : public QListWidget
{
public:
    typedef QListWidget BaseClass;

    GraphDefinitionWidget(QWidget *pParent) :
        BaseClass(pParent)
    {
        // TODO: this should be a "auto"-entry.
        this->addItem(GraphDefinitionEntry::xyGraph(QString(), QString()).toJson());
    }

    template <class Func_T>
    void forEachDefinitionEntry(Func_T handler)
    {
        for (int i = 0, nCount = count(); i < nCount; ++i)
        {
            handler(GraphDefinitionEntry::fromJson(item(i)->text()));
        }
    }
}; // Class GraphDefinitionWidget


class XySeries
{
protected:
    XySeries() {}
public:
    virtual ~XySeries() {}
    virtual void setOrAppend(const DataSourceIndex, const double, const double) = 0;
    virtual void resize(const DataSourceIndex) = 0;

    // Sets x values and y values. If given x and y ranges have different size, request is ignored.
    virtual void setValues(DFG_CLASS_NAME(RangeIterator_T)<const double*>, DFG_CLASS_NAME(RangeIterator_T)<const double*>) = 0;
}; // Class XySeries

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

    void setValues(DFG_CLASS_NAME(RangeIterator_T)<const double*> xVals, DFG_CLASS_NAME(RangeIterator_T)<const double*> yVals) override
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

class ChartCanvas
{
protected:
    ChartCanvas() {}
public:
    virtual ~ChartCanvas() {}

    virtual bool hasChartObjects() const = 0;

    virtual void addXySeries() = 0;

    virtual void setTitle(StringViewUtf8) {}

    virtual std::shared_ptr<XySeries> getFirstXySeries() { return nullptr; }

    virtual void setAxisForSeries(XySeries*, const double /*xMin*/, const double /*xMax*/, const double /*yMin*/, const double /*yMax*/) {}

}; // class ChartCanvas


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

    void setTitle(StringViewUtf8 svTitle) override
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

    std::shared_ptr<XySeries> getFirstXySeries() override
    {
        auto pChart = (m_spChartView) ? m_spChartView->chart() : nullptr;
        if (!pChart)
            return nullptr;
        auto seriesList = pChart->series();
        return (!seriesList.isEmpty()) ? std::shared_ptr<XySeries>(new XySeriesQtChart(seriesList.front())) : nullptr;
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

}} // module namespace

DFG_MODULE_NS(qt)::GraphControlPanel::GraphControlPanel(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
    auto pGraphDefinitionWidget = new GraphDefinitionWidget(this);
    pLayout->addWidget(pGraphDefinitionWidget);

    // TODO: definitions as whole must be copy-pasteable text so that it can be (re)stored easily.
}

DFG_MODULE_NS(qt)::GraphDisplay::GraphDisplay(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
    auto pChartCanvas = new ChartCanvasQtChart(this);
    auto pWidget = pChartCanvas->getWidget();
    if (pWidget)
        pLayout->addWidget(pWidget);
    m_spChartCanvas.reset(pChartCanvas);
#else
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

DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::GraphControlAndDisplayWidget()
{
    auto pLayout = new QGridLayout(this);
    auto pSplitter = new QSplitter(Qt::Vertical, this);
    m_spControlPanel.reset(new GraphControlPanel(this));
    m_spGraphDisplay.reset(new GraphDisplay(this));
    pSplitter->addWidget(m_spControlPanel.data());
    m_spControlPanel->hide(); // Hiding as it's currently non-functional
    pSplitter->addWidget(m_spGraphDisplay.data());
    //pLayout->addWidget(new QLabel(tr("Graph display"), this));
    pLayout->addWidget(pSplitter);

    this->setFrameShape(QFrame::Panel);
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refresh()
{
    // TODO: re-entrancy guard?
    // TODO: graph filling should be done in worker thread to avoid GUI freezing.

    // Going through every item in definition entry table and redrawing them.
    auto pDefWidget = this->m_spControlPanel->findChild<GraphDefinitionWidget*>();
    if (pDefWidget)
    {
        pDefWidget->forEachDefinitionEntry([&](const GraphDefinitionEntry& defEntry)
        {
            this->forDataSource(defEntry.sourceId(), [&](GraphDataSource& source)
            {
                // Checking that source type in compatible with graph type
                const auto dataType = source.dataType();
                if (dataType != GraphDataSourceType_tableSelection) // Currently only one type is supported.
                {
                    // TODO: generate log entry and/or note to user
                    DFG_ASSERT_IMPLEMENTED(false);
                    return;
                }

                if (!m_spGraphDisplay)
                {
                    DFG_ASSERT_WITH_MSG(false, "Internal error, no graph display object");
                    return;
                }
                auto pChart = this->m_spGraphDisplay->chart();
                if (!pChart)
                {
                    DFG_ASSERT_WITH_MSG(false, "Internal error, no chart object available");
                    return;
                }

                const auto sourceId = source.uniqueId();
                auto sTitle = (!sourceId.isEmpty()) ? format_fmt("Data source {}", sourceId.toUtf8().data()) : std::string();
                pChart->setTitle(SzPtrUtf8(sTitle.c_str()));

                if (!pChart->hasChartObjects())
                {
                    pChart->addXySeries();
                }
                auto spSeries = pChart->getFirstXySeries();

                if (!spSeries)
                {
                    DFG_ASSERT_WITH_MSG(false, "Internal error, unexpected series type");
                    return;
                }

                typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<double, double> RowToValueMap;
                typedef DFG_MODULE_NS(cont)::DFG_CLASS_NAME(MapVectorSoA)<DataSourceIndex, RowToValueMap> ColumnToValuesMap;

                ColumnToValuesMap colToValuesMap; // Maps column to "(row, column value)" table.

                // Fetching data from source.
                source.forEachElement_fromTableSelection([&](DataSourceIndex r, DataSourceIndex c, const QVariant& val)
                {
                    auto insertRv = colToValuesMap.insert(std::move(c), RowToValueMap());
                    if (insertRv.second) // If new RowToValueMap was added, temporarily disabling sorting.
                        insertRv.first->second.setSorting(false);
                    insertRv.first->second.m_keyStorage.push_back(static_cast<double>(r));
                    insertRv.first->second.m_valueStorage.push_back(val.toDouble());
                });
                // Sorting values by row now that all data has been added.
                for (auto iter = colToValuesMap.begin(), iterEnd = colToValuesMap.end(); iter != iterEnd; ++iter)
                {
                    iter->second.setSorting(true);
                }

                DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
                DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;
                DataSourceIndex nGraphSize = 0;

                if (colToValuesMap.size() == 1)
                {
                    const auto& valueCont = colToValuesMap.begin()->second;
                    nGraphSize = valueCont.size();
                    minMaxX = ::DFG_MODULE_NS(alg)::forEachFwd(valueCont.keyRange(), minMaxX);
                    minMaxY = ::DFG_MODULE_NS(alg)::forEachFwd(valueCont.valueRange(), minMaxY);
                    // TODO: valueCont has effectively temporaries - ideally should be possible to use existing storages
                    //       to store new content to those directly or at least allow moving these storages to series
                    //       so it wouldn't need to duplicate them.
                    spSeries->setValues(valueCont.keyRange(), valueCont.valueRange());
                }
                else if (colToValuesMap.size() == 2)
                {
                    const auto& xValueMap = colToValuesMap.frontValue();
                    const auto& yValueMap = colToValuesMap.backValue();
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
                        const auto x = xIter->second;
                        const auto y = yIter->second;
                        minMaxX(x);
                        minMaxY(y);
                        spSeries->setOrAppend(nActualSize, x, y);
                        nActualSize++;
                        ++xIter;
                        ++yIter;
                    }
                    nGraphSize = nActualSize;
                }
                spSeries->resize(nGraphSize); // Removing excess points (if any)

                pChart->setAxisForSeries(spSeries.get(), minMaxX.minValue(), minMaxX.maxValue(), minMaxY.minValue(), minMaxY.maxValue());
            });
        });
    }
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
