#include "graphTools.hpp"

#include "connectHelper.hpp"

#include "../cont/MapVector.hpp"

#include "../func/memFunc.hpp"

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
DFG_END_INCLUDE_QT_HEADERS

#include <QListWidget>
#include <QJsonDocument>
#include <QVariantMap>

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

#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
    typedef QXYSeries XySeries;

    class LineSeries : public QLineSeries
    {
    public:
        LineSeries(QObject* pParent)
            : QLineSeries(pParent)
        {}
    };

    class ChartImpl : public QChart
    {
    public:
        void setAxisForSeries(XySeries* pSeries, const double minX, const double maxX, const double minY, const double maxY)
        {
            if (!pSeries)
                return;

            if (this->axes().isEmpty())
                createDefaultAxes();

            auto xAxes = this->axes(Qt::Horizontal);
            auto yAxes = this->axes(Qt::Vertical);
            if (!xAxes.isEmpty())
                pSeries->attachAxis(xAxes.front());
            if (!yAxes.isEmpty())
                pSeries->attachAxis(yAxes.front());

            // Setting axis ranges
            {
                if (!xAxes.isEmpty() && xAxes.front())
                    xAxes.front()->setRange(minX, maxX);
                if (!yAxes.isEmpty() && yAxes.front())
                    yAxes.front()->setRange(minY, maxY);
            }
        }
    };



#else
    class GraphSeries
    {
    public:
        virtual ~GraphSeries() {}
    };

    class XySeries : public GraphSeries
    {
    public:
        DataSourceIndex count() const { return 0; }
        void replace(const DataSourceIndex, const double, const double) const { }
        void append(const double, const double) const {  }
        void removePoints(DataSourceIndex nPos, DataSourceIndex nRemoveCount)
        {
            DFG_UNUSED(nPos);
            DFG_UNUSED(nRemoveCount);
        }
    };

    class LineSeries : public XySeries
    {
    public:
        LineSeries(QObject* pParent)
        {
            DFG_UNUSED(pParent);
        }
    };

    class ChartImpl
    {
    public:
        void setTitle(QString s)
        {
            m_sTitle = std::move(s);
        }
        QList<GraphSeries*> series() { return QList<GraphSeries*>(); }
        void addSeries(GraphSeries*) {}

        void setAxisForSeries(XySeries* pSeries, const double minX, const double maxX, const double minY, const double maxY)
        {
            DFG_UNUSED(pSeries);
            DFG_UNUSED(minX);
            DFG_UNUSED(maxX);
            DFG_UNUSED(minY);
            DFG_UNUSED(maxY);
        }

        QString m_sTitle;
    };




#endif

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
    auto pChartView = new QChartView(this);
    pChartView->setRenderHint(QPainter::Antialiasing);
    pLayout->addWidget(pChartView);

    std::unique_ptr<ChartImpl> spChart(new ChartImpl);
    pChartView->setChart(spChart.release()); // ChartView takes ownership.
#else
    auto pTextEdit = new QPlainTextEdit(this);
    pTextEdit->appendPlainText(tr("Placeholder"));
    pLayout->addWidget(pTextEdit);
#endif
}

auto DFG_MODULE_NS(qt)::GraphDisplay::chart() -> ChartImpl*
{
#if defined(DFG_ALLOW_QT_CHARTS) && (DFG_ALLOW_QT_CHARTS == 1)
    auto pChartView = this->findChild<QChartView*>();
    if (!pChartView)
    {
        DFG_ASSERT_WITH_MSG(false, "Internal error, no chart view object available");
        return nullptr;
    }
    return dynamic_cast<ChartImpl*>(pChartView->chart());
#else
    DFG_ASSERT_IMPLEMENTED(false);
    return nullptr;
#endif
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
                    // TODO: generate log entry and/ or note to user
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
                const QString sTitle = (sourceId.isEmpty()) ? QString() : tr("Data source %1").arg(sourceId);
                pChart->setTitle(sTitle);

                auto listOfSeries = pChart->series();

                if (listOfSeries.isEmpty())
                {
                    pChart->addSeries(new LineSeries(m_spGraphDisplay.get())); // Chart takes ownership of series
                    //pChart->addSeries(new QScatterSeries(m_spGraphDisplay.get())); // Chart takes ownership of series
                }
                auto* pSeries = dynamic_cast<XySeries*>(pChart->series().front());

                if (!pSeries)
                {
                    DFG_ASSERT_WITH_MSG(false, "Internal error, unexpected series type");
                    return;
                }

                typedef DFG_MODULE_NS(cont)::MapVectorSoA<int, dfg::cont::MapVectorSoA<int, double>> ColumnValues;
                ColumnValues values;

                source.forEachElement_fromTableSelection([&](const DataSourceIndex r, const DataSourceIndex c, const QVariant& val)
                {
                    int rowAsInt = static_cast<int>(r);
                    values[c].insert(std::move(rowAsInt), val.toDouble());
                });

                DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxX;
                DFG_MODULE_NS(func)::MemFuncMinMax<double> minMaxY;
                DataSourceIndex nGraphSize = 0;

                const auto setOrAppend = [](XySeries& series, const int i, const double x, const double y)
                    {
                        if (i < series.count())
                            series.replace(i, x, y);
                        else
                            series.append(x, y);
                    };

                if (values.size() == 1)
                {
                    const auto& valueCont = values.begin()->second;
                    nGraphSize = static_cast<int>(valueCont.size());
                    int i = 0;
                    for (const auto valPair : valueCont)
                    {
                        const auto x = valPair.first;
                        const auto y = valPair.second;
                        minMaxX(x);
                        minMaxY(y);
                        setOrAppend(*pSeries, i++, x, y);
                    }
                }
                else if (values.size() == 2)
                {
                    const auto& xValueMap = values.frontValue();
                    const auto& yValueMap = values.backValue();
                    nGraphSize = static_cast<int>(Min(xValueMap.size(), yValueMap.size()));
                    for (int i = 0; i < nGraphSize; ++i)
                    {
                        const auto x = (xValueMap.begin() + i)->second;
                        const auto y = (yValueMap.begin() + i)->second;
                        minMaxX(x);
                        minMaxY(y);
                        setOrAppend(*pSeries, i, x, y);
                    }

                }
                // Removing excess points (if any)
                pSeries->removePoints(nGraphSize, pSeries->count() - nGraphSize);

                pChart->setAxisForSeries(pSeries, minMaxX.minValue(), minMaxX.maxValue(), minMaxY.minValue(), minMaxY.maxValue());
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
