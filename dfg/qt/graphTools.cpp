#include "graphTools.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QPlainTextEdit>
    #include <QGridLayout>
    #include <QComboBox>
    #include <QLabel>
    #include <QSplitter>

#if defined(DFG_ALLOW_QT_CHARTS) && DFG_ALLOW_QT_CHARTS == 1
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

    QJsonDocument m_items;
};

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
};

DFG_MODULE_NS(qt)::GraphControlPanel::GraphControlPanel(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
    auto pGraphDefinitionWidget = new GraphDefinitionWidget(this);
    pLayout->addWidget(pGraphDefinitionWidget);
}

DFG_MODULE_NS(qt)::GraphDisplay::GraphDisplay(QWidget *pParent) : BaseClass(pParent)
{
    auto pLayout = new QGridLayout(this);
#if defined(DFG_ALLOW_QT_CHARTS) && DFG_ALLOW_QT_CHARTS == 1
    auto pChartView = new QChartView(this);
    pChartView->setRenderHint(QPainter::Antialiasing);
    pLayout->addWidget(pChartView);

    auto pSeries = new QLineSeries(this);
    pSeries->append(0,0);
    pSeries->append(1,1);
    pSeries->append(2,2);
    pSeries->append(3,1);

    auto pChart = new QChart;
    //chart->legend()->hide();
    pChart->addSeries(pSeries);
    pChart->createDefaultAxes();
    pChart->setTitle(tr("Placeholder chart"));

    pChartView->setChart(pChart);

#else
    auto pTextEdit = new QPlainTextEdit(this);
    pTextEdit->appendPlainText(tr("Placeholder"));
    pLayout->addWidget(pTextEdit);
#endif
}

DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::GraphControlAndDisplayWidget()
{
    auto pLayout = new QGridLayout(this);
    auto pSplitter = new QSplitter(Qt::Vertical, this);
    m_spControlPanel.reset(new GraphControlPanel(this));
    pSplitter->addWidget(m_spControlPanel.data());
    pSplitter->addWidget(new GraphDisplay(this));
    pLayout->addWidget(new QLabel(tr("Graph display"), this));
    pLayout->addWidget(pSplitter);

    this->setFrameShape(QFrame::Panel);
}

void DFG_MODULE_NS(qt)::GraphControlAndDisplayWidget::refresh()
{
    // Going through every item in definition entry table and redrawing them.
    auto pDefWidget = this->m_spControlPanel->findChild<GraphDefinitionWidget*>();
    if (pDefWidget)
    {
        pDefWidget->forEachDefinitionEntry([](const GraphDefinitionEntry& defEntry)
        {
            DFG_UNUSED(defEntry);
        });
    }
}
