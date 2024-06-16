#include "graphTools_qcustomplot.hpp"
#include "../connectHelper.hpp"
#include "../widgetHelpers.hpp"
#include "../qtBasic.hpp"
#include "../../str.hpp"
#include "../qtIncludeHelpers.hpp"
#include "../../func/memFuncMedian.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QTextDocument>
DFG_END_INCLUDE_QT_HEADERS

using namespace DFG_MODULE_NS(charts)::fieldsIds;

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

namespace qcpObjectPropertyIds
{
    constexpr char statBoxCount[]                   = "dfglib_statbox_count_map";
    constexpr char graphOperationsSupported[]       = "dfglib_graph_operations_supported";  // Note that this is about applying
                                                                                            // manual operations to existing graph items, not about
                                                                                            // whether definition entry supports operations.
}

template <class T>
T getQcpObjectProperty(const QCPAbstractPlottable* pPlottable, const char* pszPropertyName, const T defaultVal)
{
    if (!pPlottable)
        return defaultVal;
    const auto var = pPlottable->property(pszPropertyName);
    return (!var.isNull() && var.canConvert<T>()) ? var.value<T>() : defaultVal;
}

template <class T>
void setQcpObjectProperty(QCPAbstractPlottable* pPlottable, const char* pszPropertyName, const T value)
{
    if (!pPlottable)
        return;
    pPlottable->setProperty(pszPropertyName, value);
}

template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T & rChartObject, ValueCont_T && yVals);

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T & rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>&xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>&yVals);

static void fillQcpPlottable(QCPAbstractPlottable * pPlottable, ::DFG_MODULE_NS(charts)::ChartOperationPipeData & pipeData);

template <class Func_T>
void forEachQCustomPlotLineStyle(const QCPGraph*, Func_T && func);

template <class Func_T>
void forEachQCustomPlotLineStyle(const QCPCurve*, Func_T && func);

template <class Func_T>
void forEachQCustomPlotScatterStyle(Func_T && func);

template <class Cont_T, class PointToText_T>
static void createNearestPointToolTipList(Cont_T & cont, const PointXy & xy, ToolTipTextStream & toolTipStream, PointToText_T pointToText); // Note: QCPDataContainer doesn't seem to have const begin()/end() so must take cont by non-const reference.

static QString axisTypeToToolTipString(const QCPAxis::AxisType axisType);

class PointToTextConverterBase
{
public:
    PointToTextConverterBase(const QCPAbstractPlottable& plottable)
    {
        const auto axisTicker = [](QCPAxis* pAxis) { return (pAxis) ? pAxis->ticker() : nullptr; };
        auto spXticker = axisTicker(plottable.keyAxis());
        auto spYticker = axisTicker(plottable.valueAxis());
        m_pXdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spXticker.data());
        m_pYdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spYticker.data());
        m_pXtextTicker = dynamic_cast<QCPAxisTickerText*>(spXticker.data());
    }

    QString xNumberText(const double val) const
    {
        return ToolTipTextStream::numberToText(val, m_pXdateTicker, m_pXtextTicker);
    }

    static QString tr(const char* psz)
    {
        return QApplication::tr(psz);
    }

    const QCPAxisTickerDateTime* m_pXdateTicker = nullptr;
    const QCPAxisTickerDateTime* m_pYdateTicker = nullptr;
    QCPAxisTickerText* m_pXtextTicker = nullptr; // Not const because of reasons noted in ToolTipTextStream::numberToText()
}; // PointToTextConverter

template <class Data_T>
class PointToTextConverter : public PointToTextConverterBase
{
public:
    using BaseClass = PointToTextConverterBase;
    PointToTextConverter(const QCPAbstractPlottable& plottable)
        : BaseClass(plottable)
    {
    }

    QString operator()(const Data_T& data, const ToolTipTextStream& toolTipStream) const
    {
        DFG_UNUSED(toolTipStream);
        return QString("(%1, %2)").arg(xText(data), yText(data));
    }

    QString xText(const Data_T& data) const
    {
        return xNumberText(data.key);
    }

    QString yText(const Data_T& data) const
    {
        return ToolTipTextStream::numberToText(data.value, m_pYdateTicker);
    }
}; // PointToTextConverter

template <>
class PointToTextConverter<QCPStatisticalBoxData> : public PointToTextConverterBase
{
public:
    using DataT = QCPStatisticalBoxData;
    using BaseClass = PointToTextConverterBase;
    PointToTextConverter<QCPStatisticalBoxData>(const QCPStatisticalBox& plottable)
        : BaseClass(plottable)
    {
        m_countMap = getQcpObjectProperty(&plottable, qcpObjectPropertyIds::statBoxCount, QVariantMap());
    }

    QString operator()(const DataT& data, const ToolTipTextStream& toolTipStream) const
    {
        DFG_UNUSED(toolTipStream);
        const qulonglong nDefaultValue = uint64_max;
        const auto nCount = m_countMap.value(QString::number(data.key), nDefaultValue).toULongLong();
        const QString sCount = (nCount != nDefaultValue) ? QString(", count: %1").arg(nCount) : QString();
        return QString("(%1; me: %2, mi: %3, ma: %4, lq: %5, uq: %6%7)")
            .arg(xNumberText(data.key))
            .arg(data.median)
            .arg(data.minimum)
            .arg(data.maximum)
            .arg(data.lowerQuartile)
            .arg(data.upperQuartile)
            .arg(sCount);
    }

    QVariantMap m_countMap;
}; // PointToTextConverter<QCPStatisticalBoxData>

namespace
{
    // Returns initial placeholder text for operation definition.
    QString getOperationDefinitionPlaceholder(const StringViewUtf8& svOperationId)
    {
        using namespace ::DFG_MODULE_NS(charts)::operations;
        const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
        if (svOperationId == PassWindowOperation::id())
            return tr("%1(x, 0, 1)").arg(viewToQString(svOperationId));
        return QString("%1()").arg(viewToQString(svOperationId));
    }

    QString getOperationDefinitionUsageGuide(const StringViewUtf8& svOperationId)
    {
        using namespace ::DFG_MODULE_NS(charts)::operations;
        const auto tr = [](const char* psz) { return QCoreApplication::tr(psz); };
        if (svOperationId == PassWindowOperation::id())
            return tr("%1(<axis>, <lower_bound>, <upper_bound>)").arg(viewToQString(svOperationId));
        return QString();
    }
} // unnamed namespace

/////////////////////////////////////////////////////////////////////////////////////
//
// ChartObjectQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////

void ChartObjectQCustomPlot::setNameImpl(const ChartObjectStringView s)
{
    if (m_spPlottable)
        m_spPlottable->setName(viewToQString(s));
}

auto ChartObjectQCustomPlot::nameImpl() const -> StringViewOrOwnerUtf8
{
    return (m_spPlottable) ? StringViewOrOwnerUtf8::makeOwned(qStringToStringUtf8(m_spPlottable->name())) : StringViewOrOwnerUtf8();
}

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

/////////////////////////////////////////////////////////////////////////////////////
//
// XySeriesQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////

XySeriesQCustomPlot::XySeriesQCustomPlot(QCPAbstractPlottable* pQcpObject)
    : m_spQcpObject(pQcpObject)
{
    setBaseImplementation<ChartObjectQCustomPlot>(m_spQcpObject.data());
}

bool XySeriesQCustomPlot::setLineStyle(QCPGraph* pGraph, StringViewC svStyle)
{
    if (!pGraph)
        return false;

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
    pGraph->setLineStyle(style);
    return true;
}

bool XySeriesQCustomPlot::setLineStyle(QCPCurve* pCurve, StringViewC svStyle)
{
    if (!pCurve)
        return false;

    QCPCurve::LineStyle style = QCPCurve::lsNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPCurve::lsLine;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown line style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));
    }
    pCurve->setLineStyle(style);
    return true;
}

void XySeriesQCustomPlot::setLineStyle(StringViewC svStyle)
{
    if (!setLineStyle(getGraph(), svStyle))
        setLineStyle(getCurve(), svStyle);
}

void XySeriesQCustomPlot::setScatterStyle(const QCPScatterStyle style)
{
    auto pGraph = getGraph();
    if (pGraph)
        pGraph->setScatterStyle(style);
    else
    {
        auto pCurve = getCurve();
        if (pCurve)
            pCurve->setScatterStyle(style);
    }
}

void XySeriesQCustomPlot::setPointStyle(StringViewC svStyle)
{
    if (!m_spQcpObject)
        return;
    QCPScatterStyle style = QCPScatterStyle::ssNone; // Note: If changing this, reflect change to logging below.
    if (svStyle == "basic")
        style = QCPScatterStyle::ssCircle;
    else if (svStyle != "none")
    {
        // Ending up here means that entry was unrecognized.
        DFG_QT_CHART_CONSOLE_WARNING(QString("Unknown point style '%1', using style 'none'").arg(untypedViewToQStringAsUtf8(svStyle)));

    }
    setScatterStyle(style);
}

size_t XySeriesQCustomPlot::setMetaDataByFunctor(MetaDataSetterCallback func)
{
    auto pCurve = dynamic_cast<CustomPlotCurveWithMetaData*>(m_spQcpObject.data());
    if (pCurve)
    {
        auto spData = pCurve->data();
        size_t nCallCount = 0;
        if (spData)
        {
            for (const auto& dataItem : *spData)
            {
                pCurve->setMetaDataStringAt(dataItem, func(dataItem.t, dataItem.key, dataItem.value));
                ++nCallCount;
            }
        }
        return nCallCount;
    }
    else
        return 0;
}

void XySeriesQCustomPlot::resize(const DataSourceIndex nNewSize)
{
    if (!resizeImpl(getGraph(), nNewSize, [](Dummy, double x, double y) { return QCPGraphData(x, y); }))
        resizeImpl(getCurve(), nNewSize, [](DataSourceIndex i, double x, double y) { return QCPCurveData(static_cast<double>(i), x, y); });
}

QCPGraph* XySeriesQCustomPlot::getGraph()
{
    return qobject_cast<QCPGraph*>(m_spQcpObject.data());
}

QCPCurve* XySeriesQCustomPlot::getCurve()
{
    return qobject_cast<QCPCurve*>(m_spQcpObject.data());
}

void XySeriesQCustomPlot::setValues(InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags)
{
    if (!setValuesImpl<QCPGraphData>(getGraph(), xVals, yVals, pFilterFlags, [](Dummy, double x, double y) { return QCPGraphData(x, y); }))
        setValuesImpl<QCPCurveData>(getCurve(), xVals, yVals, pFilterFlags, [](size_t n, double x, double y) { return QCPCurveData(static_cast<double>(n), x, y); });
}

template <class QCPObject_T, class Constructor_T>
bool XySeriesQCustomPlot::resizeImpl(QCPObject_T* pQcpObject, const DataSourceIndex nNewSize, Constructor_T constructor)
{
    if (!pQcpObject)
        return false;
    auto spData = pQcpObject->data();
    if (!spData)
        return true;
    const auto nOldSize = static_cast<DataSourceIndex>(spData->size());
    if (nOldSize == nNewSize)
        return true;
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
            spData->add(constructor(i, std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()));

    }
    return true;
}

template <class DataType_T, class QcpObject_T, class DataContructor_T>
bool XySeriesQCustomPlot::setValuesImpl(QcpObject_T* pQcpObject, InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags, DataContructor_T constructor)
{
    if (!pQcpObject)
        return false;

    if (xVals.size() != yVals.size())
        return true;

    auto iterY = yVals.cbegin();
    QVector<DataType_T> data;
    if (pFilterFlags && pFilterFlags->size() == xVals.size())
    {
        auto iterFlag = pFilterFlags->begin();
        size_t n = 0;
        for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++iterFlag, ++n)
        {
            if (*iterFlag)
                data.push_back(constructor(n, *iterX, *iterY));
        }
    }
    else
    {
        data.reserve(saturateCast<int>(xVals.size()));
        size_t n = 0;
        for (auto iterX = xVals.cbegin(), iterEnd = xVals.cend(); iterX != iterEnd; ++iterX, ++iterY, ++n)
        {
            data.push_back(constructor(n, *iterX, *iterY));
        }
    }
    fillQcpPlottable(*pQcpObject, std::move(data));
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////
//
// HistogramQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////////////
//
// BarSeriesQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////

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

/////////////////////////////////////////////////////////////////////////////////////
//
// StatisticalBoxQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////


StatisticalBoxQCustomPlot::StatisticalBoxQCustomPlot(QCPStatisticalBox* pStatBox)
{
    m_spStatBox = pStatBox;
    if (!m_spStatBox)
        return;
    setBaseImplementation<ChartObjectQCustomPlot>(m_spStatBox.data());
}

StatisticalBoxQCustomPlot::~StatisticalBoxQCustomPlot()
{
}


/////////////////////////////////////////////////////////////////////////////////////
//
// ChartCanvasQCustomPlot
//
/////////////////////////////////////////////////////////////////////////////////////

ChartCanvasQCustomPlot::ChartCanvasQCustomPlot(QWidget* pParent)
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

bool ChartCanvasQCustomPlot::hasChartObjects() const
{
    return m_spChartView && (m_spChartView->plottableCount() > 0);
}

namespace
{
    template <class QCPObject_T, class Style_T, class StyleSetter_T>
    static void addGraphStyleAction(QMenu& rMenu, QCPObject_T& rQcpObject, QCustomPlot& rCustomPlot, const Style_T currentStyle, const Style_T style, const QString& sStyleName, StyleSetter_T styleSetter)
    {
        auto pAction = rMenu.addAction(sStyleName, [=, &rQcpObject, &rCustomPlot]() { (rQcpObject.*styleSetter)(style); rCustomPlot.replot(); });
        if (pAction)
        {
            pAction->setCheckable(true);
            if (currentStyle == style)
                pAction->setChecked(true);
        }
    }

    // Returns true iff pObj was non-null
    template<class T>
    static bool addContextMenuEntriesForXyType(QCustomPlot& rQcp, QMenu& rMenu, T* pObj)
    {
        if (!pObj)
            return false;
        // Adding line style entries
        {
            addSectionEntryToMenu(&rMenu, rMenu.tr("Line Style"));
            const auto currentLineStyle = pObj->lineStyle();
            forEachQCustomPlotLineStyle(pObj, [&](const typename T::LineStyle style, const char* pszStyleName)
                {
                    addGraphStyleAction(rMenu, *pObj, rQcp, currentLineStyle, style, rMenu.tr(pszStyleName), &T::setLineStyle);
                });
        }

        // Adding point style entries
        {
            addSectionEntryToMenu(&rMenu, rMenu.tr("Point Style"));
            const auto currentPointStyle = pObj->scatterStyle().shape();
            forEachQCustomPlotScatterStyle([&](const QCPScatterStyle::ScatterShape style, const char* pszStyleName)
                {
                    addGraphStyleAction(rMenu, *pObj, rQcp, currentPointStyle, style, rMenu.tr(pszStyleName), &T::setScatterStyle);
                });
        }
        return true;
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
        //const auto name = pPlottable->name();

        // TODO: limit length
        // TODO: icon based on object type (xy, histogram...)
        auto pSubMenu = menu.addMenu(pPlottable->name());
        if (!pSubMenu)
            continue;

        // Adding menu title
        addTitleEntryToMenu(pSubMenu, pPlottable->name());

        // Adding remove-entry
        pSubMenu->addAction(tr("Remove"), m_spChartView.get(), [=]() { m_spChartView->removePlottable(pPlottable); repaintCanvas(); });

        // Adding operations-menu if plottable supports those
        if (getQcpObjectProperty(pPlottable, qcpObjectPropertyIds::graphOperationsSupported, true))
        {
            auto pOperationsMenu = pSubMenu->addMenu(tr("Operations"));
            if (pOperationsMenu)
            {
                QPointer<QCPAbstractPlottable> spPlottable = pPlottable;
                pOperationsMenu->addAction(tr("Apply multiple..."), pPlottable, [=]() { applyChartOperationsTo(spPlottable); });
                pOperationsMenu->addSeparator();
                DFG_DETAIL_NS::operationManager().forEachOperationId([&](StringUtf8 sOperationId)
                    {
                        // TODO: add operation only if it accepts input type that chart object provides.
                        pOperationsMenu->addAction(viewToQString(sOperationId), pPlottable, [=]() { applyChartOperationTo(spPlottable, sOperationId); });
                    });

            }
        }

        if (addContextMenuEntriesForXyType(*m_spChartView, *pSubMenu, qobject_cast<QCPGraph*>(pPlottable)))
            continue;
        if (addContextMenuEntriesForXyType(*m_spChartView, *pSubMenu, qobject_cast<QCPCurve*>(pPlottable)))
            continue;

        auto pBars = qobject_cast<QCPBars*>(pPlottable);
        if (pBars)
        {
            addSectionEntryToMenu(pSubMenu, tr("Bar controls not implemented"));
            continue;
        }
    }
}

void ChartCanvasQCustomPlot::removeAllChartObjects(const bool bRepaint)
{
    auto p = getWidget();
    if (!p)
        return;
    p->clearPlottables();

    // Removing legends
    removeLegends();

    // Removing panels
    auto pPlotLayout = p->plotLayout();
    if (pPlotLayout)
    {
        for (int i = 0, nCount = pPlotLayout->elementCount(); i < nCount; ++i)
            pPlotLayout->removeAt(i);
        auto pFirstPanel = (pPlotLayout->elementCount() >= 1) ? dynamic_cast<ChartPanel*>(pPlotLayout->elementAt(0)) : nullptr;
        if (pFirstPanel)
            pFirstPanel->setTitle(StringViewUtf8());
        pPlotLayout->simplify(); // This removes the empty space that removed items left.

        // Removing axis labels
        forEachAxisRect([&](QCPAxisRect& axisRect)
            {
                forEachAxis(&axisRect, [](QCPAxis& rAxis)
                    {
                        rAxis.setLabel(QString());
                        // Resetting colours
                        ChartCanvasQCustomPlot::resetPanelAxisColour(rAxis);
                        ChartCanvasQCustomPlot::resetPanelAxisLabelColour(rAxis);
                    });
            });
    }
    if (bRepaint)
        repaintCanvas();
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
        // TODO: if there are multiple graphs in the same panel, identical axis may get recreated several times.
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

void ChartCanvasQCustomPlot::setTypeToQcpObjectProperty(QCPAbstractPlottable* pPlottable, const StringViewC& type)
{
    if (pPlottable)
        pPlottable->setProperty("chartEntryType", untypedViewToQStringAsUtf8(type));
}

auto ChartCanvasQCustomPlot::createXySeries(const XySeriesCreationParam& param) -> ChartObjectHolder<XySeries>
{
    auto p = getWidget();
    if (!p)
        return nullptr;

    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;
    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to obtain axis for xySeries"));
        return nullptr;
    }

    const auto type = param.definitionEntry().graphTypeStr();

    QCPGraph* pQcpGraph = nullptr;
    QCPCurve* pQcpCurve = nullptr;
    if (type == ChartObjectChartTypeStr_xy)
    {
        pQcpGraph = p->addGraph(pXaxis, pYaxis);
        setTypeToQcpObjectProperty(pQcpGraph, type);
    }
    else if (type == ChartObjectChartTypeStr_txy)
    {
        pQcpCurve = new QCPCurve(pXaxis, pYaxis); // Owned by QCustomPlot
        setTypeToQcpObjectProperty(pQcpCurve, type);
    }
    else if (type == ChartObjectChartTypeStr_txys)
    {
        pQcpCurve = new CustomPlotCurveWithMetaData(pXaxis, pYaxis); // Owned by QCustomPlot
        setTypeToQcpObjectProperty(pQcpCurve, type);
    }
    else
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Invalid type '%1'").arg(untypedViewToQStringAsUtf8(type.asUntypedView())));
        return nullptr;
    }

    if (!pQcpGraph && !pQcpCurve)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Internal error: failed to create QCP-object"));
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
    if (pQcpGraph)
        return ChartObjectHolder<XySeries>(new XySeriesQCustomPlot(pQcpGraph));
    else
        return ChartObjectHolder<XySeries>(new XySeriesQCustomPlot(pQcpCurve));
}

auto ChartCanvasQCustomPlot::createHistogram(const HistogramCreationParam& param) -> ChartObjectHolder<Histogram>
{
    // If histogram is string-type (=bin per strings, values are number of identical strings), handling it separately - it is effectively a bar chart.
    if (!param.stringValueRange.empty())
    {
        DFG_ASSERT_CORRECTNESS(param.stringValueRange.size() == param.countRange.size());
        auto barSeries = createBarSeries(BarSeriesCreationParam(param.config(), param.definitionEntry(), param.stringValueRange, param.countRange, param.xType, param.m_sXname, StringUtf8()));
        DFG_ASSERT_CORRECTNESS(barSeries.size() <= 1);
        auto spImpl = (!barSeries.empty()) ? dynamic_cast<BarSeriesQCustomPlot*>(barSeries[0].get()) : nullptr;
        return (spImpl) ? std::make_shared<HistogramQCustomPlot>(spImpl->m_spBars.data()) : nullptr;
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

    std::unique_ptr<QCPBars> spQCPBars(new QCPBars(pXaxis, pYaxis));
    setTypeToQcpObjectProperty(spQCPBars.get(), ChartObjectChartTypeStr_histogram);
    auto spHistogram = std::make_shared<HistogramQCustomPlot>(spQCPBars.release()); // Note: QCPBars is owned by QCustomPlot-object.

    const bool bOnlySingleValue = (*minMaxPair.first == *minMaxPair.second);

    const auto nBinCount = (!bOnlySingleValue) ? param.definitionEntry().fieldValue<int>(ChartObjectFieldIdStr_binCount, 100) : -1;

    spHistogram->setValues(param.valueRange, param.countRange);

    double binWidth = std::numeric_limits<double>::quiet_NaN();

    if (nBinCount >= 0)
        binWidth = (*minMaxPair.second - *minMaxPair.first) / static_cast<double>(nBinCount);
    else // Case: bin for every value.
    {
        const auto keyRange = makeRange(param.valueRange);
        if (keyRange.size() >= 2)
        {
            const auto iterEnd = keyRange.cend();
            double minDiff = std::numeric_limits<double>::infinity();
            for (auto iter = keyRange.cbegin(), iterNext = keyRange.cbegin() + 1; iterNext != iterEnd; ++iter, ++iterNext)
                minDiff = Min(minDiff, *iterNext - *iter);
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

auto ChartCanvasQCustomPlot::createBarSeries(const BarSeriesCreationParam& param) -> std::vector<ChartObjectHolder<BarSeries>>
{
    using ReturnType = std::vector<ChartObjectHolder<BarSeries>>;
    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;

    if (!pXaxis || !pYaxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create histogram, no suitable target panel found'"));
        return ReturnType();
    }

    auto labelRange = param.labelRange;
    auto valueRange = param.valueRange;

    if (labelRange.empty() || labelRange.size() != valueRange.size())
        return ReturnType();

    auto minMaxPair = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(valueRange);
    if (*minMaxPair.first > *minMaxPair.second || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.first) || !DFG_MODULE_NS(math)::isFinite(*minMaxPair.second))
        return ReturnType();

    // Handling bar merging if requested
    QVector<StringUtf8> adjustedLabels; // label buffer that is used for if bars need to be merged
    QVector<double> yAdjustedData; // y value buffer that is used for if bars need to be merged
    const auto bMergeIdentical = param.definitionEntry().fieldValue(ChartObjectFieldIdStr_mergeIdenticalLabels, false);
    if (bMergeIdentical)
    {
        ::DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, double> uniqueValues;
        uniqueValues.setSorting(false); // Want to keep original order.
        for (size_t i = 0, nCount = valueRange.size(); i < nCount; ++i)
            uniqueValues[labelRange[i]] += valueRange[i];
        if (uniqueValues.size() != valueRange.size()) // Does data have duplicate bar identifiers?
        {
            DFG_REQUIRE(valueRange.size() >= uniqueValues.size());
            const auto nRemoveCount = valueRange.size() - uniqueValues.size();
            const auto nNewLabelSize = saturateCast<int>(valueRange.size() - nRemoveCount);
            adjustedLabels.resize(saturateCast<int>(nNewLabelSize));
            yAdjustedData.resize(saturateCast<int>(uniqueValues.size()));
            int i = 0;
            for (const auto& item : uniqueValues)
            {
                adjustedLabels[i] = item.first;
                yAdjustedData[i] = item.second;
                i++;
            }
            // Recalculating y-range.
            const auto minMaxIters = ::DFG_MODULE_NS(numeric)::minmaxElement_withNanHandling(yAdjustedData);
            const auto iterToPointer = [&](const auto iter) { return yAdjustedData.data() + (iter - yAdjustedData.begin()); };
            minMaxPair.first = iterToPointer(minMaxIters.first);
            minMaxPair.second = iterToPointer(minMaxIters.second);
            labelRange = adjustedLabels;
            valueRange = yAdjustedData;
        }
    }

    QVector<double> ticks;
    QVector<QString> labels;

    // Checking if there is existing text ticker and if yes, prepending existing ticks and labels before new items.
    auto pExistingTextTicker = dynamic_cast<QCPAxisTickerText*>(pXaxis->ticker().data()); // QCPAxisTicker is not a QObject (at least in 2.0.1) so can't use qobject_cast
    if (pExistingTextTicker)
    {
        const auto& tickerTicks = pExistingTextTicker->ticks();
        for (auto iter = tickerTicks.cbegin(), iterEnd = tickerTicks.end(); iter != iterEnd; ++iter)
        {
            ticks.push_back(iter.key());
            labels.push_back(iter.value());
        }
    }

    ::DFG_MODULE_NS(cont)::ValueVector<double> stackedValues;
    // Checking if bars in current data should be stacked under identical label.
    const auto sBarLabel = param.definitionEntry().fieldValueStr(ChartObjectFieldIdStr_barLabel);
    if (!sBarLabel.empty())
    {
        stackedValues.assign(valueRange);
        adjustedLabels.clear();
        adjustedLabels.push_back(sBarLabel);
        labelRange = adjustedLabels;
    }

    const bool bStackBars = param.definitionEntry().fieldValue(ChartObjectFieldIdStr_stackOnExistingLabels, false);

    QCPBars* pStackBarsOn = nullptr;

    if (bStackBars)
    {
        // If stacking is enabled, finding last existing bar chart; stacking is done on top of that.
        const auto plottables = pXaxis->plottables();
        for (const auto& pPlottable : plottables)
        {
            auto pBars = qobject_cast<QCPBars*>(pPlottable);
            if (!pBars || pPlottable->property("chartEntryType").toString() != ChartObjectChartTypeStr_bars)
                continue;
            pStackBarsOn = pBars;
        }
    }

    QMap<QString, double> mapExistingLabelToXcoordinate;

    // If stacking is enabled, finding existing labels.
    if (pStackBarsOn)
    {
        auto pTextTicker = dynamic_cast<QCPAxisTickerText*>(pXaxis->ticker().data()); // QCPAxisTicker is not a QObject (at least in 2.0.1) so can't use qobject_cast
        if (pTextTicker)
        {
            const auto& tickerTicks = pTextTicker->ticks();
            for (auto iter = tickerTicks.cbegin(), iterEnd = tickerTicks.end(); iter != iterEnd; ++iter)
            {
                mapExistingLabelToXcoordinate[iter.value()] = iter.key(); // Note: if there are identical labels, picks last occurence.
            }
        }
    }

    // Filling x-data; ticks and labels.
    const auto existingTickCount = labels.size();
    QVector<double> xValues;
    for (size_t i = 0, nCount = labelRange.size(); i < nCount; ++i)
    {
        const auto& sNewLabel = labelRange[i];
        auto sqNewLabel = viewToQString(sNewLabel);
        auto iter = mapExistingLabelToXcoordinate.find(sqNewLabel);
        if (iter == mapExistingLabelToXcoordinate.end()) // If using stacking and label already exists, not adding label or tick; using existing instead.
        {
            // Existing label not found -> adding new
            labels.push_back(std::move(sqNewLabel));
            ticks.push_back(static_cast<double>(ticks.size() + 1)); // Note: other parts of code rely on this 1-based indexing; To find dependent code, search for BARS_INDEX_1
            xValues.push_back(ticks.back());
        }
        else
            xValues.push_back(iter.value()); // Identical label already exists -> using it's x-value
    }

    DFG_REQUIRE(existingTickCount <= ticks.size());

    // Setting text ticker
    {
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        textTicker->addTicks(ticks, labels);
        pXaxis->setTicker(textTicker);
    }

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

    if (stackedValues.empty())
    {
        auto pBars = new QCPBars(pXaxis, pYaxis); // Note: QCPBars is owned by QCustomPlot-object.
        setTypeToQcpObjectProperty(pBars, param.definitionEntry().graphTypeStr());
        DFG_ASSERT_CORRECTNESS(static_cast<size_t>(xValues.size()) == valueRange.size());
        fillQcpPlottable<QCPBarsData>(*pBars, xValues, valueRange);
        pBars->moveAbove(pStackBarsOn);
        ReturnType rv;
        rv.push_back(std::make_shared<BarSeriesQCustomPlot>(pBars));
        return rv;
    }
    else
    {
        ReturnType rv;
        size_t i = 0;
        for (const auto val : stackedValues)
        {
            auto pBars = new QCPBars(pXaxis, pYaxis); // Note: QCPBars is owned by QCustomPlot-object.
            pBars->setName(viewToQString(param.labelRange[i++]));
            setTypeToQcpObjectProperty(pBars, param.definitionEntry().graphTypeStr());
            fillQcpPlottable<QCPBarsData>(*pBars, xValues, makeRange(&val, &val + 1));
            pBars->moveAbove(pStackBarsOn);
            pStackBarsOn = pBars;
            rv.push_back(std::make_shared<BarSeriesQCustomPlot>(pBars));
        }
        return rv;
    }
}

auto ChartCanvasQCustomPlot::createStatisticalBox(const StatisticalBoxCreationParam& param) -> std::vector<ChartObjectHolder<StatisticalBox>>
{
    if (param.labelRange.size() != param.valueRanges.size())
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create Statistical box: internal error, label and value range size mismatch'"));
        return {};
    }

    if (param.labelRange.empty())
        return {};

    auto pXaxis = getXAxis(param);
    auto pYaxis = (pXaxis) ? getYAxis(param) : nullptr;
    const auto pChartPanel = this->getChartPanelByAxis(pXaxis);

    if (!pXaxis || !pYaxis || !pChartPanel)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Failed to create Statistical box, no suitable target panel found'"));
        return {};
    }
    
    const auto nInitialStatBoxCount = pChartPanel->countOf(ChartObjectChartTypeStr_statBox);

    QCPStatisticalBox* pEffectiveQCPStatBox = nullptr;

    const double xPosOffset = [&]()
        {
            // Taking initial x from biggest x of existing stat boxes + 1.
            ::DFG_MODULE_NS(func)::MemFuncMax<double> xMax(0);
            pChartPanel->forEachChartObject([&](const QCPAbstractPlottable& rPlottable)
                {
                    const auto pStatBox = qobject_cast<const QCPStatisticalBox*>(&rPlottable);
                    if (!pStatBox)
                        return;
                    const auto spData = pStatBox->data();
                    if (!spData || spData->isEmpty())
                        return;
                    xMax(spData->at(spData->size() - 1)->key);
                });
            return xMax.value() + 1;
        }();

    std::unique_ptr<QCPStatisticalBox> spQCPStatBox;
    if (!pEffectiveQCPStatBox)
    {
        spQCPStatBox.reset(new QCPStatisticalBox(pXaxis, pYaxis));
        setTypeToQcpObjectProperty(spQCPStatBox.get(), ChartObjectChartTypeStr_statBox);
        pEffectiveQCPStatBox = spQCPStatBox.get();
    }
     
    QCPAxisTickerText* pEffectiveTextTicker = dynamic_cast<QCPAxisTickerText*>(pXaxis->ticker().data());
    QSharedPointer<QCPAxisTickerText> spNewTextTicker;
    if (pEffectiveTextTicker == nullptr)
    {
        spNewTextTicker.reset(new QCPAxisTickerText);
        pEffectiveTextTicker = spNewTextTicker.data();
    }

    QVector<double> keys;
    QVector<double> minimums;
    QVector<double> lowerQuartiles;
    QVector<double> medians;
    QVector<double> upperQuartiles;
    QVector<double> maximums;

    auto countMap = getQcpObjectProperty(pEffectiveQCPStatBox, qcpObjectPropertyIds::statBoxCount, QVariantMap());

    for (size_t i = 0; i < param.labelRange.size(); ++i)
    {
        const auto valueRange = param.valueRanges[i];
        using namespace ::DFG_MODULE_NS(func);
        MemFuncPercentile_enclosingElem<double> memFuncQuart25(25);
        MemFuncMedian<double> memFuncQuart50;
        MemFuncPercentile_enclosingElem<double> memFuncQuart75(75);
        MemFuncMinMax<double> memFuncMinMax;
        size_t nCounter = 0;
        for (auto d : valueRange)
        {
            if (::DFG_MODULE_NS(math)::isNan(d))
                continue; // Skipping NaN's
            memFuncQuart25(d);
            memFuncQuart50(d);
            memFuncQuart75(d);
            memFuncMinMax(d);
            ++nCounter;
        }

        const double xPosNumber = xPosOffset + static_cast<double>(i);
        keys.push_back(xPosNumber);
        minimums.push_back(memFuncMinMax.minValue());
        lowerQuartiles.push_back(memFuncQuart25.percentile());
        medians.push_back(memFuncQuart50.median());
        upperQuartiles.push_back(memFuncQuart75.percentile());
        maximums.push_back(memFuncMinMax.maxValue());
        countMap.insert(QString::number(xPosNumber), qulonglong(nCounter));

        pEffectiveTextTicker->addTick(xPosNumber, viewToQString(param.labelRange[i]));
    }

    pEffectiveQCPStatBox->addData(keys, minimums, lowerQuartiles, medians, upperQuartiles, maximums);
    if (spQCPStatBox.get() != nullptr)
        spQCPStatBox->setName(tr("Stat box ") + QString::number(nInitialStatBoxCount + 1));
    if (spNewTextTicker)
        pXaxis->setTicker(spNewTextTicker);
    
    setQcpObjectProperty(pEffectiveQCPStatBox, qcpObjectPropertyIds::statBoxCount, countMap);
    setQcpObjectProperty(pEffectiveQCPStatBox, qcpObjectPropertyIds::graphOperationsSupported, false);
    std::vector<ChartObjectHolder<StatisticalBox>> rv;
    if (spQCPStatBox)
        rv.push_back(ChartObjectHolder<StatisticalBox>(new StatisticalBoxQCustomPlot(spQCPStatBox.release())));
    else
        rv.push_back(ChartObjectHolder<StatisticalBox>(new StatisticalBoxQCustomPlot(pEffectiveQCPStatBox)));
    return rv;
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
    if (!p)
        return;
    if (m_spUpdateIndicator)
        p->removeItem(m_spUpdateIndicator);
    p->replot();
}

int ChartCanvasQCustomPlot::width() const
{
    auto pCustomPlot = getWidget();
    return (pCustomPlot) ? pCustomPlot->width() : 0;
}

int ChartCanvasQCustomPlot::height() const
{
    auto pCustomPlot = getWidget();
    return (pCustomPlot) ? pCustomPlot->height() : 0;
}

void ChartCanvasQCustomPlot::setBackground(const StringViewUtf8& sv)
{
    auto pCustomPlot = getWidget();
    if (!pCustomPlot)
        return;

    bool bSetDefault = true;
    auto defaultBackgroundSetter = makeScopedCaller([] {}, [&]() { if (bSetDefault) pCustomPlot->setBackground(QBrush(Qt::white, Qt::SolidPattern)); });

    if (sv.empty())
        return;

    auto args = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem::fromStableView(sv);

    if (args.key() != DFG_UTF8("gradient_linear"))
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Unrecognized background definition '%1'").arg(viewToQString(args.key())));
        return;
    }

    // gradient_linear expects: gradient_linear(direction, linear position1, color1, [linear position 2, color2...])
    if (args.valueCount() < 3)
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Too few points for gradient_linear background: expected at least 3, got %1").arg(args.valueCount()));
        return;
    }

    const auto svDirection = args.value(0);
    if (svDirection != DFG_UTF8("vertical") && svDirection != DFG_UTF8("default")) // Currently only vertical and default are supported, to support horizontal and numeric angle.
    {
        DFG_QT_CHART_CONSOLE_ERROR(tr("Unsupported gradient direction '%1'").arg(viewToQString(svDirection)));
        return;
    }

    const auto nEffectiveArgCount = args.valueCount() - (1 - args.valueCount() % 2);
    if (args.valueCount() % 2 == 0)
        DFG_QT_CHART_CONSOLE_WARNING(tr("Uneven argument count for gradient_linear, last item ignored"));

    QLinearGradient gradient(0, 0, 0, this->height());
    for (size_t i = 1; i < nEffectiveArgCount; i += 2)
    {
        const auto linearPos = args.valueAs<double>(i);
        if (::DFG_MODULE_NS(math)::isNan(linearPos) || linearPos < 0 || linearPos > 1)
        {
            DFG_QT_CHART_CONSOLE_WARNING(tr("Expected linear gradient position [0, 1], got '%1', item ignored").arg(viewToQString(args.value(i))));
            continue;
        }
        const QColor colour(viewToQString(args.value(i + 1)));
        if (!colour.isValid())
        {
            DFG_QT_CHART_CONSOLE_WARNING(tr("Unable to parse colour '%1', linear gradient item ignored").arg(viewToQString(args.value(i + 1))));
            continue;
        }
        gradient.setColorAt(linearPos, colour);
    }
    bSetDefault = false;
    pCustomPlot->setBackground(QBrush(gradient));
}

void ChartCanvasQCustomPlot::setPanelAxesColour(StringViewUtf8 svPanelId, StringViewUtf8 svColourDef)
{
    auto pPanel = getChartPanel(svPanelId);
    if (!pPanel)
        return;
    const QColor color(viewToQString(svColourDef));
    pPanel->forEachAxis([&](QCPAxis& axis)
        {
            setPanelAxisColour(axis, color);
        });
}

void ChartCanvasQCustomPlot::setPanelAxisColour(QCPAxis& axis, const QColor& color)
{
    axis.setBasePen(QPen(color));
    axis.setTickPen(QPen(color));
    axis.setSubTickPen(QPen(color));
}

void ChartCanvasQCustomPlot::resetPanelAxisColour(QCPAxis& axis)
{
    setPanelAxisColour(axis, QColor(Qt::black));
}

void ChartCanvasQCustomPlot::setPanelAxesLabelColour(StringViewUtf8 svPanelId, StringViewUtf8 svColourDef)
{
    auto pPanel = getChartPanel(svPanelId);
    if (!pPanel)
        return;
    const QColor color(viewToQString(svColourDef));
    pPanel->forEachAxis([&](QCPAxis& axis)
        {
            setPanelAxisLabelColour(axis, color);
        });
}

void ChartCanvasQCustomPlot::setAxisProperties(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId, const ArgList& args)
{
    auto pAxis = this->getAxis(svPanelId, svAxisId);
    if (!pAxis)
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("No axis '%1' found from panel '%2': setting properties ignored").arg(viewToQString(svAxisId), viewToQString(svPanelId)));
        return;
    }

    const auto setRangeHelper = [&](const NonNullCStr pszAxisPropId, const StringViewSzUtf8& svPropValue)
    {
        const auto val = ::DFG_MODULE_NS(str)::strTo<double>(svPropValue);
        if (::DFG_MODULE_NS(math)::isFinite(val))
            pAxis->setProperty(pszAxisPropId, val);
        else
            DFG_QT_CHART_CONSOLE_WARNING(tr("Invalid range start '%1', unable to convert to value").arg(viewToQString(svPropValue)));
    };

    for (size_t i = 0, nArgCount = args.valueCount(); i < nArgCount; i += 2)
    {
        using namespace ::DFG_MODULE_NS(charts);
        const auto svPropId = args.value(i);
        const auto svPropIdUntyped = svPropId.asUntypedView();
        const auto svPropValue = args.value(i + 1);
        const auto svPropValueUntyped = svPropValue.asUntypedView();

        if (svPropIdUntyped == ChartObjectFieldIdStr_axisProperty_lineColour)
            setPanelAxisColour(*pAxis, QColor(viewToQString(svPropValue)));
        else if (svPropIdUntyped == ChartObjectFieldIdStr_axisProperty_labelColour)
            setPanelAxisLabelColour(*pAxis, QColor(viewToQString(svPropValue)));
        else if (svPropIdUntyped == ChartObjectFieldIdStr_axisProperty_rangeStart)
            setRangeHelper("dfglib_range_start", svPropValue);
        else if (svPropIdUntyped == ChartObjectFieldIdStr_axisProperty_rangeEnd)
            setRangeHelper("dfglib_range_end", svPropValue);
        else if (svPropIdUntyped == ChartObjectFieldIdStr_axisProperty_tickLabelFormat)
        {
            const auto nColonPos = ::DFG_MODULE_NS(alg)::indexOf(svPropValueUntyped, ':');
            const auto svFormatType = (isValidIndex(svPropValueUntyped, nColonPos)) ? svPropValueUntyped.substr_startCount(0, nColonPos) : svPropValueUntyped;
            const auto svFormatDetail = (isValidIndex(svPropValueUntyped, nColonPos)) ? svPropValueUntyped.substr_start(nColonPos + 1) : StringViewC();
            if (svFormatType == "number")
            {
                setAxisTicker(*pAxis, ChartDataType::unknown);
                if (!svFormatDetail.empty())
                    DFG_QT_CHART_CONSOLE_INFO(tr("Number format specifier in '%1' is currently ignored (axis '%2' in panel '%3')").
                        arg(viewToQString(svPropValue), viewToQString(svAxisId), viewToQString(svPanelId)));
                // QCPAxis has following functions readily available for controlling number format: setNumberFormat(), setNumberPrecision()
            }
            else if (svFormatType == "datetime")
            {
                createDateTimeTicker(*pAxis, QLatin1String(svFormatDetail.data(), svFormatDetail.sizeAsInt()));
            }
            else
                DFG_QT_CHART_CONSOLE_WARNING(tr("Unrecognized %4 '%1' for axis '%2' in panel '%3'")
                    .arg(viewToQString(svPropValue), viewToQString(svAxisId), viewToQString(svPanelId), ChartObjectFieldIdStr_axisProperty_tickLabelFormat));
        }
        else
            DFG_QT_CHART_CONSOLE_WARNING(tr("Unrecognized axis property '%1' for axis '%2' in panel '%3'").arg(viewToQString(svPropId), viewToQString(svAxisId), viewToQString(svPanelId)));
    }
}

void ChartCanvasQCustomPlot::setPanelAxisLabelColour(QCPAxis& axis, const QColor& color)
{
    axis.setTickLabelColor(color);
    axis.setLabelColor(color);
}

void ChartCanvasQCustomPlot::resetPanelAxisLabelColour(QCPAxis& axis)
{
    setPanelAxisLabelColour(axis, QColor(Qt::black));
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)) // Not sure is version accurate, introduced for Qt 5.9
            pLayout->addElement(pLegend, Qt::AlignTop | Qt::AlignRight);
#else
            pLayout->addElement(pLegend, static_cast<Qt::Alignment>(Qt::AlignTop | Qt::AlignRight));
#endif
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

template <class Func_T> void ChartCanvasQCustomPlot::forEachAxisRect(Func_T&& func) { forEachAxisRectImpl(*this, std::forward<Func_T>(func)); }
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

template <class This_T, class Func_T>
void ChartCanvasQCustomPlot::forEachChartPanelUntil(This_T& rThis, Func_T&& func)
{
    auto pCustomPlot = rThis.getWidget();
    auto pMainLayout = (pCustomPlot) ? pCustomPlot->plotLayout() : nullptr;
    if (!pMainLayout)
        return;
    const auto nElemCount = pMainLayout->elementCount();
    using ChartPanelPtr = std::conditional_t<std::is_const_v<This_T>, const ChartPanel*, ChartPanel*>;
    for (int i = 0; i < nElemCount; ++i)
    {
        auto pElement = pMainLayout->elementAt(i);
        auto pPanel = dynamic_cast<ChartPanelPtr>(pElement);
        if (pPanel)
            if (!func(*pPanel))
                break;
    }
}

template <class Func_T>
void ChartCanvasQCustomPlot::forEachChartPanelUntil(Func_T&& func)
{
    forEachChartPanelUntil(*this, std::forward<Func_T>(func));
}

template <class Func_T>
void ChartCanvasQCustomPlot::forEachChartPanelUntil(Func_T&& func) const
{
    forEachChartPanelUntil(*this, std::forward<Func_T>(func));
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
    
    // Testing existince first as otherwise QCPLayoutGrid::element() causes debug log noise "Invalid row. Row: x Column: y"
    ChartPanel* pChartPanel = pLayout->hasElement(nRow, nCol) ? dynamic_cast<ChartPanel*>(pLayout->element(nRow, nCol)) : nullptr;
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

template <class This_T>
auto ChartCanvasQCustomPlot::getChartPanelImpl(This_T& rThis, const QPoint& pos) -> std::conditional_t<std::is_const_v<This_T>, const ChartPanel*, ChartPanel*>
{
    const auto isWithinAxisRange = [](const QCPAxis* pAxis, const double val)
    {
        return (pAxis) ? pAxis->range().contains(val) : false;
    };

    using ChartPanelPtr = std::conditional_t<std::is_const_v<This_T>, const ChartPanel*, ChartPanel*>;
    ChartPanelPtr pPanel = nullptr;
    rThis.forEachChartPanelUntil([&](decltype(*ChartPanelPtr())& panel)
        {
            auto pXaxis1 = panel.primaryXaxis();
            auto pYaxis1 = panel.primaryYaxis();
            if (pXaxis1 && pYaxis1)
            {
                const auto xCoord = pXaxis1->pixelToCoord(pos.x());
                const auto yCoord = pYaxis1->pixelToCoord(pos.y());
                if (isWithinAxisRange(pXaxis1, xCoord) && isWithinAxisRange(pYaxis1, yCoord))
                    pPanel = &panel;
            }
            return pPanel == nullptr;
        });
    return pPanel;
}

auto ChartCanvasQCustomPlot::getChartPanel(const QPoint& pos) -> ChartPanel*
{
    return getChartPanelImpl(*this, pos);
}

auto ChartCanvasQCustomPlot::getChartPanel(const QPoint& pos) const -> const ChartPanel*
{
    return getChartPanelImpl(*this, pos);
}

auto ChartCanvasQCustomPlot::getChartPanel(const StringViewUtf8& svPanelId) -> ChartPanel*
{
    auto pCustomPlot = getWidget();
    auto pMainLayout = (pCustomPlot) ? pCustomPlot->plotLayout() : nullptr;
    if (!pMainLayout)
        return nullptr;

    int nRow = 0;
    int nCol = 0;
    if (!getGridPos(svPanelId, nRow, nCol))
        return nullptr;
    return dynamic_cast<ChartPanel*>(pMainLayout->element(nRow, nCol));
}

auto ChartCanvasQCustomPlot::getChartPanelByAxis(const QCPAxis* pAxis) -> ChartPanel*
{
    ChartPanel* pPanel = nullptr;
    forEachChartPanelUntil([&](ChartPanel& rPanel)
        {
            if (rPanel.hasAxis(pAxis))
                pPanel = &rPanel;
            return pPanel == nullptr;
        });
    return pPanel;
}

auto ChartCanvasQCustomPlot::getAxis(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId) -> QCPAxis*
{
    auto pAxisRect = getAxisRect(svPanelId);
    if (svAxisId == DFG_UTF8("x"))
        return pAxisRect->axis(QCPAxis::atBottom);
    else if (svAxisId == DFG_UTF8("y"))
        return pAxisRect->axis(QCPAxis::atLeft);
    else if (svAxisId == DFG_UTF8("y2"))
        return pAxisRect->axis(QCPAxis::atRight);

    DFG_QT_CHART_CONSOLE_WARNING(tr("Didn't find axis '%1' from panel '%2'").arg(viewToQString(svAxisId), viewToQString(svPanelId)));
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
    using namespace ::DFG_MODULE_NS(charts);
    bool bAxisIdPresent = false;
    const auto sAxisId = param.definitionEntry().fieldValueStr(ChartObjectFieldIdStr_yAxisId, &bAxisIdPresent);
    if (!bAxisIdPresent || sAxisId == DFG_UTF8("y"))
        return getAxis(param, QCPAxis::atLeft);
    else if (sAxisId == DFG_UTF8("y2"))
    {
        auto pAxis = getAxis(param, QCPAxis::atRight);
        if (pAxis)
            pAxis->setVisible(true);
        return pAxis;
    }
    else
    {
        DFG_QT_CHART_CONSOLE_WARNING(tr("Invalid axis id '%1'. Expected either 'y' or 'y2'").arg(viewToQString(sAxisId)));
        return nullptr;
    }
}

void ChartCanvasQCustomPlot::setPanelTitle(StringViewUtf8 svPanelId, StringViewUtf8 svTitle, StringViewUtf8 svTitleColor)
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
        pPanel->setTitle(svTitle, svTitleColor);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// ChartCanvasQCustomPlot::BarStack
//
/////////////////////////////////////////////////////////////////////////////////////

void ChartCanvasQCustomPlot::BarStack::storeStackLabel(const QCPBarsData& data, const QCPBars& rBars)
{
    m_sLabel = PointToTextConverter<QCPBarsData>(rBars).xText(data);
    m_xValue = data.key;
}

QString ChartCanvasQCustomPlot::BarStack::label() const
{
    return m_sLabel;
}

const QCPAxis* ChartCanvasQCustomPlot::BarStack::valueAxis() const
{
    auto pBars = barsInstance();
    return (pBars) ? pBars->valueAxis() : nullptr;
}

const QCustomPlot* ChartCanvasQCustomPlot::BarStack::parentPlot() const
{
    auto pBars = barsInstance();
    return (pBars) ? pBars->parentPlot() : nullptr;
}

const QCPBars* ChartCanvasQCustomPlot::BarStack::barsInstance() const
{
    return (!m_subBarInfos.empty()) ? m_subBarInfos.front().m_spBars.data() : nullptr;
}

double ChartCanvasQCustomPlot::BarStack::barWidth() const
{
    auto pBar = barsInstance();
    return (pBar) ? pBar->width() : std::numeric_limits<double>::quiet_NaN();
}

bool ChartCanvasQCustomPlot::BarStack::isXwithinStackX(const double x) const
{
    const auto barHalfWidth = barWidth() / 2;
    return x >= (this->m_xValue - barHalfWidth) && x <= (this->m_xValue + barHalfWidth);
}

void ChartCanvasQCustomPlot::BarStack::forEachSubBar(const PointXy& pointXy, ToolTipTextStream& toolTipStream, SubBarHandler handler) const
{
    DFG_UNUSED(toolTipStream);
    if (!handler)
        return;

    const auto pQcp = parentPlot();

    for (const auto& subBarInfo : m_subBarInfos)
    {
        const QCPBars* pBar = subBarInfo.m_spBars;
        if (!pBar)
            continue;
        const bool bIsPointWithIn = [&]()
        {
            if (pQcp && isXwithinStackX(pointXy.first))
            {
                auto pKeyAxis = pBar->keyAxis();
                auto pValueAxis = pBar->valueAxis();
                if (pKeyAxis && pValueAxis)
                {
                    // Testing if cursor is within given bar.
                    const auto xPix = pKeyAxis->coordToPixel(pointXy.first);
                    const auto yPix = pValueAxis->coordToPixel(pointXy.second);
                    const double selectDistance = pBar->selectTest(QPointF(xPix, yPix), false);
                    if (selectDistance >= 0 && selectDistance < pQcp->selectionTolerance())
                        return true;
                }
            }
            return false;
        }();

        handler(subBarInfo.m_sLabel,
            PointToTextConverter<QCPBarsData>(*pBar).yText(subBarInfo.m_data),
            pBar->pen().color().name(),
            bIsPointWithIn);
    }
}

bool ChartCanvasQCustomPlot::BarStack::hasObject(const QCPBars& bars) const
{
    for (const auto& info : m_subBarInfos)
    {
        if (info.m_spBars.data() == &bars)
            return true;
    }
    return false;
}

const QCPBars& ChartCanvasQCustomPlot::BarStack::findTopMostBar(const QCPBars& rBars)
{
    auto pBars = &rBars;
    while (pBars->barAbove() != nullptr)
        pBars = pBars->barAbove();
    return *pBars;
}

void ChartCanvasQCustomPlot::BarStack::append(const QCPBarsData& data, const QCPBars& rBars)
{
    auto pBars = &findTopMostBar(rBars);
    for (; pBars != nullptr; pBars = pBars->barBelow())
    {
        // Fetching data (=height) for current bar.
        auto spData = pBars->data();
        DFG_ASSERT_CORRECTNESS(spData != nullptr); // To detect if this ever happens
        if (spData)
        {
            auto iterData = spData->findBegin(data.sortKey(), false);
            DFG_ASSERT_CORRECTNESS(iterData != spData->end() && iterData->key == data.key);
            if (iterData != spData->end() && iterData->key == data.key)
                m_subBarInfos.push_back(SubBarInfo(*iterData, *pBars));
        }
    }
    storeStackLabel(data, rBars);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// ChartCanvasQCustomPlot::BarStacks
//
/////////////////////////////////////////////////////////////////////////////////////

void ChartCanvasQCustomPlot::BarStacks::storeStack(const QCPBars* pBars)
{
    // Not doing anything if any of following:
    //  -No object given
    //  -Related stacks already stored
    //  -Object has no below or above bar.
    if (!pBars || hasBar(*pBars) || (pBars->barAbove() == nullptr && pBars->barBelow() == nullptr))
        return;

    auto spData = pBars->data();
    if (!spData || spData->isEmpty())
        return;
    for (const auto& data : *spData)
    {
        m_mapPosToBarStack[data.key].append(data, *pBars);
    }
}

// Returns BarStacks related to given QCPBars-object
auto ChartCanvasQCustomPlot::BarStacks::findStacks(const QCPBars& bars) const -> std::vector<const BarStack*>
{
    std::vector<const BarStack*> stacks;
    for (const auto& stack : m_mapPosToBarStack.valueRange())
    {
        if (stack.hasObject(bars))
            stacks.push_back(&stack);
    }
    return stacks;
}

// Returns true iff given QCPBars is within any stored stack.
bool ChartCanvasQCustomPlot::BarStacks::hasBar(const QCPBars& bars) const
{
    return !findStacks(bars).empty();
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& xy, ToolTipTextStream& toolTipStream)
{
    if (!pGraph)
        return false;

    toolTipStream << tr("<br>Graph size: %1").arg(pGraph->dataCount());

    const auto spData = pGraph->data();
    if (!spData)
        return true;

    createNearestPointToolTipList(*spData, xy, toolTipStream, PointToTextConverter<QCPGraphData>(*pGraph));

    return true;
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPCurve* pCurve, const PointXy& cursorXy, ToolTipTextStream& toolTipStream) const
{
    const auto spPoints = (pCurve) ? pCurve->data() : nullptr;
    if (!spPoints)
        return false;

    auto& rCurve = *pCurve;

    toolTipStream << tr("<br>Graph size: %1").arg(rCurve.dataCount());

    // Unlike in QCPGraph, now adjacent points in index space can be arbitrarily far from each other in both x and y axis.
    // So in practice need to go through all points to find nearest and this time showing nearest by distance between cursor and point,
    // not just x-coordinate distance between them.

    enum class DistanceType { raw, axisNormalized, pixel };

    struct DistanceAndPoint
    {
        DistanceAndPoint(double d2, QCPCurveData curveData)
            : distanceSquare(d2)
            , data(curveData)
        {}
        bool operator<(const DistanceAndPoint& other) const { return this->distanceSquare < other.distanceSquare; }
        double distanceSquare;
        QCPCurveData data;
    };

    const auto pow2 = [](const double val) { return val * val; };

    const auto distanceType = DistanceType::pixel;

    const auto nWidth = this->width();
    const auto nHeight = this->height();

    auto pKeyAxis = rCurve.keyAxis();
    auto pValueAxis = rCurve.valueAxis();

    const auto xRange = (pKeyAxis) ? pKeyAxis->range().size() : std::numeric_limits<double>::quiet_NaN();
    const auto yRange = (pValueAxis) ? pValueAxis->range().size() : std::numeric_limits<double>::quiet_NaN();

    const auto xWidthFactor = (distanceType == DistanceType::pixel) ? nWidth / xRange : 1.0 / xRange;
    const auto yHeightFactor = (distanceType == DistanceType::pixel) ? nHeight / yRange : 1.0 / yRange;

    const auto distanceSquare = [&](const QCPCurveData& xy) { return pow2(xy.key - cursorXy.first) + pow2(xy.value - cursorXy.second); };
    const auto normalizedSquare = [&](const QCPCurveData& xy)
    {
        // Problem with raw distance is that if other axis is covering, say, [0, 1] and the other [1e5, 1e6], nearest points to cursor are typically
        // those closest on y-axis. However for tooltip, visual distance is probably what user is expecting since tooltip follows mouse cursor.
        // AxisNormalized is closer, but corresponds to visual only if chart canvas is a square, pixel-distance takes also this into account.
        const auto normalizedPow2 = [&](const double a, const double b, const double factor)
        {
            return pow2(factor * (a - b));
        };
        return normalizedPow2(xy.key, cursorXy.first, xWidthFactor) + normalizedPow2(xy.value, cursorXy.second, yHeightFactor);
    };

    const auto nToolTipPointCount = static_cast<size_t>(Min(5, spPoints->size()));
    const auto pMetaCurve = dynamic_cast<const CustomPlotCurveWithMetaData*>(pCurve);

    const auto distanceFunc = [&](const QCPCurveData& data)
        {
            if (distanceType == DistanceType::raw)
                return distanceSquare(data);
            else
                return normalizedSquare(data);
        };

    ::DFG_MODULE_NS(cont)::SortedSequence<std::vector<DistanceAndPoint>> nearest;
    std::for_each(spPoints->constBegin(), spPoints->constEnd(), [&](const QCPCurveData& data)
        {
            const auto d2 = distanceFunc(data);
            if (nearest.size() >= nToolTipPointCount)
            {
                if (d2 < nearest.back().distanceSquare)
                {
                    nearest.pop_back();
                    nearest.insert(DistanceAndPoint(d2, data));
                }
            }
            else
                nearest.insert(DistanceAndPoint(d2, data));
        });

    auto pointToText = PointToTextConverter<QCPCurveData>(*pCurve);

    const QString sDistanceDescription = [&distanceType]()
        { 
            if (distanceType == DistanceType::axisNormalized)
                return tr("axis normalized ");
            else if (distanceType == DistanceType::pixel)
                return tr("pixel ");
            else
                return QString();
        }();
    toolTipStream << tr("<br>Nearest points and %1distance to cursor:").arg(sDistanceDescription);
    for (const auto& item : nearest)
    {
        if (pMetaCurve)
            toolTipStream << QString("<br>%1: %2 (%3)").arg(pointToText(item.data, toolTipStream), viewToQString(pMetaCurve->metaDataStringAt(item.data))).arg(std::sqrt(item.distanceSquare));
        else
            toolTipStream << QString("<br>%1 (%2)").arg(pointToText(item.data, toolTipStream)).arg(std::sqrt(item.distanceSquare));
    }

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

    PointToTextConverter<QCPBarsData> pointToText(*pBars);
    createNearestPointToolTipList(*spData, xy, toolTipStream, pointToText);

    return true;
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const BarStack* pBarStack, const PointXy& cursorXy, ToolTipTextStream& toolTipStream)
{
    if (!pBarStack)
        return false;

    auto& rBarStack = *pBarStack;

    const auto sStackLabel = rBarStack.label();

    // Stack label (x value label)
    toolTipStream << tr("<br>Bar stack: <b>%1</b>").arg(sStackLabel.toHtmlEscaped());
    // Axis identifier
    auto pValueAxis = rBarStack.valueAxis();
    if (pValueAxis && pValueAxis->axisType() == QCPAxis::atRight) // Printing y-axis info if using non-default.
        toolTipStream << tr("<br>y axis: %1").arg(axisTypeToToolTipString(pValueAxis->axisType()));

    toolTipStream << tr("<br>Bar count: %1").arg(pBarStack->size());

    rBarStack.forEachSubBar(cursorXy, toolTipStream, [&](const QString& sLabel, const QString& sValue, const QColor& color, const bool bIsCursorWithin)
        {
            toolTipStream << QString("<br>%4<font color=\"%2\">'%1'</font>: %3%5").arg(
                sLabel.toHtmlEscaped(),
                color.name(),
                sValue,
                (bIsCursorWithin) ? "<b>" : "",
                (bIsCursorWithin) ? "</b>" : "");
        });

    return true;
}

bool ChartCanvasQCustomPlot::toolTipTextForChartObjectAsHtml(const QCPStatisticalBox* pStatBox, const PointXy& cursorXy, ToolTipTextStream& toolTipStream)
{
    if (!pStatBox)
        return false;

    auto& rStatBox = *pStatBox;
    const auto spData = rStatBox.data();
    if (!spData)
        return true;

    toolTipStream << tr("<br>Box count: %1").arg(rStatBox.dataCount());

    PointToTextConverter<QCPStatisticalBoxData> pointToText(*pStatBox);
    createNearestPointToolTipList(*spData, cursorXy, toolTipStream, pointToText);
    return true;
}

auto ChartCanvasQCustomPlot::createToolTipTextAtImpl(const int x, const int y, const ToolTipTextRequestFlags flags) const -> StringUtf8
{
    if (!flags.testFlag(ToolTipTextRequestFlags::html) && !flags.testFlag(ToolTipTextRequestFlags::plainText))
        return StringUtf8(); // Caller required an unsupported type -> return empty.
    const QPoint cursorPos(x, y);
    auto pPanel = getChartPanel(cursorPos);

    if (!pPanel)
        return StringUtf8();

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

    BarStacks barStacks;

    pPanel->forEachChartObject([&](const QCPAbstractPlottable& plottable)
        {
            barStacks.storeStack(qobject_cast<const QCPBars*>(&plottable));
        });

    QSet<const BarStack*> handledStacks;

    // For each chart object in this panel
    pPanel->forEachChartObject([&](const QCPAbstractPlottable& plottable)
        {
            constexpr char szSeparator[] = "<br>---------------------------";

            // Handling bar stacks first.
            auto pBars = qobject_cast<const QCPBars*>(&plottable);
            if (pBars)
            {
                auto stacks = barStacks.findStacks(*pBars);
                if (!stacks.empty())
                {
                    for (const auto pStack : stacks)
                    {
                        if (!handledStacks.contains(pStack)) // Handling this object only if related stack not handled already.
                        {
                            toolTipStream << szSeparator;
                            toolTipTextForChartObjectAsHtml(pStack, xy, toolTipStream);
                            handledStacks.insert(pStack);
                        }
                    }
                    return;
                }
            }

            toolTipStream << szSeparator;

            // Name
            toolTipStream << QString("<br><font color=\"%2\">'%1'</font>").arg(plottable.name().toHtmlEscaped(), plottable.pen().color().name());
            // Axis identifier
            {
                const auto pValueAxis = plottable.valueAxis();
                if (pValueAxis && pValueAxis->axisType() == QCPAxis::atRight) // Printing y-axis info if using non-default.
                    toolTipStream << tr("<br>y axis: %1").arg(axisTypeToToolTipString(pValueAxis->axisType()));
            }
            if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPGraph*>(&plottable), xy, toolTipStream)) {}
            else if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPCurve*>(&plottable), xy, toolTipStream)) {}
            else if (toolTipTextForChartObjectAsHtml(pBars, xy, toolTipStream)) {}
            else if (toolTipTextForChartObjectAsHtml(qobject_cast<const QCPStatisticalBox*>(&plottable), xy, toolTipStream)) {}
        });
    auto sToolTip = toolTipStream.toPlainText();
    if (!flags.testFlag(ChartCanvas::ToolTipTextRequestFlags::html))
    {
        // If caller requested plain text, converting generated HTML to such using QTextDocument
        QTextDocument document;
        document.setHtml(sToolTip);
        sToolTip = document.toPlainText();
    }
    return qStringToStringUtf8(sToolTip);
}

void ChartCanvasQCustomPlot::mouseMoveEvent(QMouseEvent* pEvent)
{
    if (!pEvent || !m_bToolTipEnabled)
        return;

    const auto cursorPos = pEvent->pos();

    const auto sText = createToolTipTextAt(cursorPos.x(), cursorPos.y());

    const auto sQStringText = viewToQString(sText);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QToolTip::showText(pEvent->globalPosition().toPoint(), sQStringText);
#else
    QToolTip::showText(pEvent->globalPos(), sQStringText);
#endif
}

namespace
{
    template <class Cont_T>
    auto createOperationPipeDataImpl(const QSharedPointer<Cont_T> spData) -> ::DFG_MODULE_NS(charts)::ChartOperationPipeData
    {
        using namespace ::DFG_MODULE_NS(charts);
        if (!spData)
            return ChartOperationPipeData();

        ChartOperationPipeData pipeData;
        auto px = pipeData.editableValuesByIndex(0);
        auto py = pipeData.editableValuesByIndex(1);
        if (!px || !py)
        {
            DFG_ASSERT(false); // Not expected to ever end up here.
            return pipeData;
        }
        const auto nSize = static_cast<size_t>(spData->size());
        px->resize(nSize);
        py->resize(nSize);
        size_t i = 0;
        for (auto xy : *spData)
        {
            (*px)[i] = xy.key;
            (*py)[i] = xy.value;
            i++;
        }
        pipeData.setValueVectorsAsData();
        return pipeData;
    }

    auto createOperationPipeDataForBars(QCPBars& bars) -> ::DFG_MODULE_NS(charts)::ChartOperationPipeData
    {
        using namespace ::DFG_MODULE_NS(charts);

        auto pLabelAxis = bars.keyAxis();
        // Note: casting to non-const since there's no const-overload for QCPAxisTickerText::ticks().
        auto pLabelTicker = dynamic_cast<QCPAxisTickerText*>((pLabelAxis) ? pLabelAxis->ticker().data() : nullptr);
        if (!pLabelTicker)
            return ChartOperationPipeData();
        auto pData = bars.data().data();
        if (!pData)
            return ChartOperationPipeData();

        const auto& mapXvalueToLabelStr = pLabelTicker->ticks();

        ChartOperationPipeData pipeData;
        auto px = pipeData.editableStringsByIndex(0);
        auto py = pipeData.editableValuesByIndex(1);
        if (!px || !py)
        {
            DFG_ASSERT(false); // Not expected to ever end up here.
            return pipeData;
        }
        const auto nSize = static_cast<size_t>(pData->size());
        px->resize(nSize);
        py->resize(nSize);
        size_t i = 0;
        for (auto xy : *pData)
        {
            (*px)[i] = qStringToStringUtf8(mapXvalueToLabelStr.value(xy.key, QString()));
            (*py)[i] = xy.value;
            i++;
        }
        pipeData.setDataRefs(px, py);
        return pipeData;
    }
}

auto ChartCanvasQCustomPlot::createOperationPipeData(QCPAbstractPlottable* pPlottable) -> ::DFG_MODULE_NS(charts)::ChartOperationPipeData
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!pPlottable)
        return ChartOperationPipeData();
    auto pGraph = qobject_cast<QCPGraph*>(pPlottable);
    if (pGraph)
        return createOperationPipeDataImpl(pGraph->data());
    auto pCurve = qobject_cast<QCPCurve*>(pPlottable);
    if (pCurve)
        return createOperationPipeDataImpl(pCurve->data());
    auto pBars = qobject_cast<QCPBars*>(pPlottable);
    if (pBars)
    {
        if (pPlottable->property("chartEntryType").toString() == ChartObjectChartTypeStr_bars)
            return createOperationPipeDataForBars(*pBars);
        else
            return createOperationPipeDataImpl(pBars->data());
    }
    auto pStatBox = qobject_cast<QCPStatisticalBox*>(pPlottable);
    if (pStatBox)
    {
        DFG_QT_CHART_CONSOLE_WARNING(pStatBox->tr("Operations are not available for existing stat_box items"));
        return ChartOperationPipeData();
    }
    DFG_ASSERT_IMPLEMENTED(false);
    return ChartOperationPipeData();
}

void ChartCanvasQCustomPlot::applyChartOperationTo(QPointer<QCPAbstractPlottable> spPlottable, const StringViewUtf8& svOperationId)
{
    using namespace ::DFG_MODULE_NS(charts);
    QCPAbstractPlottable* pPlottable = spPlottable.data();
    if (!pPlottable)
        return;

    QString sOperationDefinition;
    // Asking arguments from user
    {
        sOperationDefinition = QInputDialog::getText(this->m_spChartView.data(),
            tr("Applying operation"),
            tr("Define operation for '%1'\nUsage: %2").arg(pPlottable->name(), getOperationDefinitionUsageGuide(svOperationId)),
            QLineEdit::Normal,
            getOperationDefinitionPlaceholder(svOperationId));
        if (sOperationDefinition.isEmpty())
            return;
    }
    applyChartOperationsTo(pPlottable, QStringList(sOperationDefinition));
}

void ChartCanvasQCustomPlot::applyChartOperationsTo(QPointer<QCPAbstractPlottable> spPlottable)
{
    using namespace ::DFG_MODULE_NS(charts);
    QCPAbstractPlottable* pPlottable = spPlottable.data();
    if (!pPlottable)
        return;

    // Asking operation list from user
    // TODO: showing operation guide in the UI would be nice.
    QString sInitial;
    bool bGoodOperationList = false;
    do
    {
        QStringList operationStringList;
        const QString sAdditionInfo = (qobject_cast<QCPBars*>(pPlottable) != nullptr)
            ? tr("\nNote: bar chart string labels are not passed to operations")
            : QString();
        bool bOk;
        sInitial = QInputDialog::getMultiLineText(this->m_spChartView.data(),
            tr("Applying operations"),
            tr("Define operations (one per line) for '%1'\n%2").arg(pPlottable->name(), sAdditionInfo),
            sInitial,
            &bOk);
        operationStringList = sInitial.split('\n');
        if (!bOk || operationStringList.isEmpty())
            return;
        bGoodOperationList = applyChartOperationsTo(pPlottable, operationStringList);
    } while (!bGoodOperationList);
}

bool ChartCanvasQCustomPlot::applyChartOperationsTo(QCPAbstractPlottable* pPlottable, const QStringList& operationStringList)
{
    auto& manager = DFG_DETAIL_NS::operationManager();
    ChartEntryOperationList operations;
    QString sErrors;
    for (const auto& sItem : operationStringList)
    {
        if (sItem.isEmpty() || sItem[0] == '#')
            continue;
        StringUtf8 sDef(SzPtrUtf8(sItem.toUtf8()));
        auto op = manager.createOperation(sDef);
        if (op)
        {
            op.m_sDefinition = std::move(sDef);
            operations.push_back(std::move(op));
        }
        else
        {
            sErrors += tr("\n'%1': ").arg(sItem);
            if (sItem.indexOf("(") != -1 && manager.hasOperation(SzPtrUtf8(sItem.mid(0, sItem.indexOf("(")).toUtf8())))
                sErrors += tr("invalid arguments");
            else
                sErrors += tr("no such operation");
        }
    }
    if (!sErrors.isEmpty())
    {
        QMessageBox::information(getWidget(), tr("Unable to create operations"), tr("Unable to create operations, the following errors were encountered:\n%1").arg(sErrors));
        return false;
    }
    applyChartOperationsTo(pPlottable, operations);
    return true;
}

void ChartCanvasQCustomPlot::applyChartOperationsTo(QCPAbstractPlottable* pPlottable, ChartEntryOperationList& operations)
{
    using namespace ::DFG_MODULE_NS(charts);

    if (!pPlottable || operations.empty())
        return;

    // Note: could use less coarse-grained implementation here for better performance: currently fills pipe data for all axes
    //       even if operations use only some. For example with bars transforms all ticker QStrings to StringUtf8 vector
    //       and later back to ticker QStrings even if operations only changes y-values.
    auto pipeData = createOperationPipeData(pPlottable);
    if (pipeData.vectorCount() == 0)
    {
        return;
    }
    operations.executeAll(pipeData);

    // Checking for errors
    QString sErrors;
    for (const auto& op : operations)
    {
        if (op.hasErrors())
        {
            sErrors += tr("\nOperation '%1': %2").arg(viewToQString(op.m_sDefinition), DFG_DETAIL_NS::formatOperationErrorsForUserVisibleText(op));
        }
    }
    if (!sErrors.isEmpty())
    {
        QMessageBox::information(getWidget(),
            tr("Errors in operations"),
            tr("There were errors applying operations, changes were not applied to chart object '%1'\n\nList of errors:\n%2").arg(pPlottable->name(), sErrors));
        return;
    }

    auto pBars = qobject_cast<QCPBars*>(pPlottable);
    // If dealing with bars, must handle x-axis and labels separately.
    if (pBars && pPlottable->property("chartEntryType").toString() == ChartObjectChartTypeStr_bars)
    {
        // fillQcpPlottable() expects to have numbers in both x and y pipes, but for bars x-pipe should have strings at this point.
        // Constructing number-valued x-pipe and reconstructing ticker labels.
        auto pLabelAxis = pBars->keyAxis();
        auto pLabelTicker = dynamic_cast<QCPAxisTickerText*>((pLabelAxis) ? pLabelAxis->ticker().data() : nullptr);
        if (pLabelTicker)
        {
            QMap<double, QString> newTicks;
            auto pStrings = pipeData.constStringsByIndex(0);
            auto pPipeXvals = pipeData.editableValuesByIndex(0);
            if (pStrings && pPipeXvals)
            {
                pPipeXvals->resize(pStrings->size());
                for (size_t i = 0; i < pStrings->size(); ++i)
                {
                    const double xVal = static_cast<double>(i + 1); // Bars are created starting with index 1, so that why +1. To find dependent code, search for BARS_INDEX_1
                    newTicks.insert(xVal, viewToQString((*pStrings)[i]));
                    (*pPipeXvals)[i] = xVal;
                }
                pipeData.setValueVectorsAsData();
            }
            pLabelTicker->setTicks(newTicks);
        }
    }

    fillQcpPlottable(pPlottable, pipeData);

    // Replotting so that the result comes visible.
    m_spChartView->replot();
}

void ChartCanvasQCustomPlot::optimizeAllAxesRanges()
{
    auto pQcp = getWidget();
    if (!pQcp)
        return;

    // Scaling doesn't seem to work correctly if plotting hasn't been made yet. So if that seems to be the case, replotting first.
    bool bDoReplotFirst = false;
    forEachAxisRect([&](QCPAxisRect& rect)
        {
            bDoReplotFirst = bDoReplotFirst || (rect.width() == 0);
        });

    if (bDoReplotFirst)
        pQcp->replot();

    pQcp->rescaleAxes();

    // Setting axis ranges and adding margin, by default margin is added so that min/max point markers won't get clipped by axisRect.
    forEachAxisRect([](QCPAxisRect& rect)
        {
            forEachAxis(&rect, [](QCPAxis& axis)
                {
                    const auto propRangeStart = axis.property("dfglib_range_start");
                    const auto propRangeEnd = axis.property("dfglib_range_end");
                    // Note scaling must be done before settings ranges in order to have range edge at given positition instead of being shifted by margin.
                    axis.scaleRange(1.1);
                    const auto oldRange = axis.range();
                    auto newRange = oldRange;
                    // Not using setRangeLower/Upper() because if e.g. old range is (-10, 10) and one calls setRangeLower(20), the range ends up being (10, 20)
                    if (isVariantType(propRangeStart, QMetaType::Double))
                    {
                        newRange.lower = propRangeStart.toDouble();
                        if (newRange.upper < newRange.lower)
                            newRange.upper = newRange.lower + 1;
                    }
                    if (isVariantType(propRangeEnd, QMetaType::Double))
                    {
                        newRange.upper = propRangeEnd.toDouble();
                        if (newRange.upper < newRange.lower)
                            newRange.lower = newRange.upper - 1;
                    }
                    if (oldRange != newRange)
                        axis.setRange(newRange);
                });
        });

    pQcp->replot();
}

void ChartCanvasQCustomPlot::beginUpdateState()
{
    auto pQcp = getWidget();
    if (!pQcp)
        return;
    if (m_spUpdateIndicator)
    {
        DFG_ASSERT_CORRECTNESS(false); // beginUpdateState() getting called twice before replot? Not a fatal error, but asserting just to make sure it get's noticed during development.
        return;
    }

    if (pQcp->axisRectCount() > 0) // If count was zero, creating QCPItemText would generate debug log noise "invalid axis rect index 0"
    {
        m_spUpdateIndicator = new QCPItemText(pQcp); // QCustomPlot-object takes ownership.
        auto pUpdateIndicator = m_spUpdateIndicator.data();
        pUpdateIndicator->setClipToAxisRect(false); // If clipToAxisRect is true, text would show only within one axis rect; in practice means that typically works only when there's just one panel.
        pUpdateIndicator->setText(tr("Updating..."));
        pUpdateIndicator->setLayer("legend"); // Setting text to legend layer so that it shows above graph stuff (axes etc.).
        // Placing text in the middle of canvas.
        {
            pUpdateIndicator->position->setType(QCPItemPosition::ptViewportRatio);
            pUpdateIndicator->position->setCoords(0.5, 0.5);
        }
        // Increasing font size
        adjustWidgetFontProperties(pUpdateIndicator, 15);
        // Setting text background so that text shows better.
        pUpdateIndicator->setBrush(QBrush(QColor(255, 255, 255, 200))); // Somewhat transparent white background.
    }

    pQcp->setBackground(QBrush(QColor(240, 240, 240))); // Setting light grey background during update.
    pQcp->replot();
}

auto ChartCanvasQCustomPlot::findPanelOfChartObject(const ChartObject* pObj) -> ::DFG_MODULE_NS(charts)::ChartPanel*
{
    auto * pObject = (pObj) ? dynamic_cast<const ChartObjectQCustomPlot*>(pObj->implementationObject()) : nullptr;
    if (!pObject)
        return nullptr;
    auto pQcpPlottable = pObject->qcpPlottable();
    if (!pQcpPlottable)
        return nullptr;
    auto pQcp = getWidget();
    if (!pQcp)
        return nullptr;

    return getChartPanelByAxis(pQcpPlottable->keyAxis());
}

/////////////////////////////////////////////////////////////////////////////////////
//
// ChartPanel
//
/////////////////////////////////////////////////////////////////////////////////////

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

auto ChartPanel::axisRect() const -> const QCPAxisRect*
{
    const auto nElemCount = elementCount();
    return (nElemCount > 0) ? qobject_cast<QCPAxisRect*>(elementAt(nElemCount - 1)) : nullptr;
}

auto ChartPanel::axis(AxisT::AxisType axisType)       -> AxisT* { return ChartPanel::axisImpl(*this, axisType); }
auto ChartPanel::axis(AxisT::AxisType axisType) const -> const AxisT* { return ChartPanel::axisImpl(*this, axisType); }

auto ChartPanel::primaryXaxis()       -> AxisT* { return axis(AxisT::atBottom); }
auto ChartPanel::primaryXaxis() const -> const AxisT* { return axis(AxisT::atBottom); }

auto ChartPanel::primaryYaxis()       ->       AxisT* { return axis(AxisT::atLeft); }
auto ChartPanel::primaryYaxis() const -> const AxisT* { return axis(AxisT::atLeft); }

auto ChartPanel::secondaryXaxis() -> AxisT*
{
    return nullptr;
}

auto ChartPanel::secondaryYaxis() -> AxisT*
{
    return nullptr;
}

bool ChartPanel::hasAxis(const QCPAxis* pAxis)
{
    return pAxis == primaryXaxis() || pAxis == primaryYaxis() || pAxis == secondaryXaxis() || pAxis == secondaryYaxis();
}

QString ChartPanel::getTitle() const
{
    auto pTitle = qobject_cast<QCPTextElement*>(element(0, 0));
    return (pTitle) ? pTitle->text() : QString();
}

void ChartPanel::setTitle(StringViewUtf8 svTitle, StringViewUtf8 svColor)
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
        if (!svColor.empty())
        {
            const QColor color(viewToQString(svColor));
            if (color.isValid())
                pTitle->setTextColor(color);
        }
    }
}

auto ChartPanel::pixelToCoord_primaryAxis(const QPoint& pos) const -> PairT
{
    auto pXaxis = primaryXaxis();
    auto pYaxis = primaryYaxis();
    const auto x = (pXaxis) ? pXaxis->pixelToCoord(pos.x()) : std::numeric_limits<double>::quiet_NaN();
    const auto y = (pYaxis) ? pYaxis->pixelToCoord(pos.y()) : std::numeric_limits<double>::quiet_NaN();
    return PairT(x, y);
}

void ChartPanel::forEachChartObject(std::function<void(const QCPAbstractPlottable&)> handler) const
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

auto ChartPanel::countOf(::DFG_MODULE_NS(charts)::AbstractChartControlItem::FieldIdStrViewInputParam type) const -> uint32
{
    auto pPrimaryX = primaryXaxis();
    if (!pPrimaryX)
        return 0;
    uint32 nCount = 0;
    const auto plottables = pPrimaryX->plottables();
    const auto sQstringType = QString::fromUtf8(type.data(), type.sizeAsInt());
    for (const auto& pPlottable : plottables)
    {
        if (!pPlottable)
            continue;
        nCount += pPlottable->property("chartEntryType").toString() == sQstringType;
    }
    return nCount;
}

auto ChartPanel::findExistingOfType(::DFG_MODULE_NS(charts)::AbstractChartControlItem::FieldIdStrViewInputParam type) const -> QCPAbstractPlottable*
{
    auto pPrimaryX = primaryXaxis();
    if (!pPrimaryX)
        return 0;
    const auto sQstringType = QString::fromUtf8(type.data(), type.sizeAsInt());
    const auto plottables = pPrimaryX->plottables();
    for (const auto& pPlottable : plottables)
    {
        if (!pPlottable)
            continue;
        if (pPlottable->property("chartEntryType").toString() == sQstringType)
            return pPlottable;
    }
    return nullptr;
}

template <class This_T>
auto ChartPanel::axisImpl(This_T& rThis, AxisT::AxisType axisType) -> decltype(rThis.axis(axisType))
{
    auto pAxisRect = rThis.axisRect();
    return (pAxisRect) ? pAxisRect->axis(axisType) : nullptr;
}


/////////////////////////////////////////////////////////////////////////////////////
//
// GraphDisplayImageExportDialog
//
/////////////////////////////////////////////////////////////////////////////////////

template <class T> T propertyValueAs(const QJsonObject&, const char*);
template <> bool    propertyValueAs<bool>(const QJsonObject& obj, const char* psz) { return obj.value(psz).toVariant().toBool(); }
template <> int     propertyValueAs<int>(const QJsonObject& obj, const char* psz) { return obj.value(psz).toVariant().toInt(); }
template <> double  propertyValueAs<double>(const QJsonObject& obj, const char* psz) { return obj.value(psz).toVariant().toDouble(); }
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

#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)) // Not sure is version accurate, introduced for Qt 5.9
    pHelpWidget->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
#else
    pHelpWidget->setTextInteractionFlags(static_cast<Qt::TextInteractionFlags>(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
#endif

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
        QMessageBox::information(this, nullptr, tr("Successfully exported to path<br>") + QString("<a href=\"file:///%1\">%1</a>").arg(sOutputPath.toHtmlEscaped()));
    }
    else
    {
        addLog(LogType::error, tr("Export failed."));
        return;
    }

    BaseClass::accept();
}

/////////////////////////////////////////////////////////////////////////////////////
//
// ToolTipTextStream
//
/////////////////////////////////////////////////////////////////////////////////////

ToolTipTextStream& ToolTipTextStream::operator<<(const QString& str)
{
    m_sText += str;
    return *this;
}

const QString& ToolTipTextStream::toPlainText() const
{
    return m_sText;
}

QString ToolTipTextStream::numberToText(const double d)
{
    return QString::number(d);
}

QString ToolTipTextStream::numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker)
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

QString ToolTipTextStream::numberToText(const double d, QCPAxisTickerText* pTextTicker) // Note: pTextTicker param is not const because QCPAxisTickerText doesn't seem to provide const-way of asking ticks.
{
    return (!pTextTicker) ? numberToText(d) : pTextTicker->ticks().value(d);
}

QString ToolTipTextStream::numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker, QCPAxisTickerText* pTextTicker)
{
    if (pTimeTicker)
        return numberToText(d, pTimeTicker);
    else if (pTextTicker)
        return numberToText(d, pTextTicker);
    else
        return numberToText(d);
}

/////////////////////////////////////////////////////////////////////////////////////
//
// Miscellaneous
//
/////////////////////////////////////////////////////////////////////////////////////

template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, ValueCont_T&& data)
{
    // Note the terminology (using QCPGraph as example)
    //  QCPGraph().data() == QSharedPointer<QCPGraphDataContainer> -> DataContainer == QCPGraphDataContainer == QCPDataContainer<QCPGraphData>
    //  QCPGraph().data()->set() takes QVector<QCPGraphData> which is the actual storage, this is ValueCont_T
    using DataContainer = typename decltype(rChartObject.data())::value_type;
    QSharedPointer<DataContainer> spData(new DataContainer);
    const auto sortFunc = qcpLessThanSortKey<typename ValueCont_T::value_type>;
    if (!std::is_sorted(data.begin(), data.end(), sortFunc))
        std::sort(data.begin(), data.end(), sortFunc);
    spData->set(data, true); // Note: if data is not sorted beforehand, sorting in set() will effetively cause a redundant copy to be created.
    rChartObject.setData(std::move(spData));
}

template <class DataType_T, class ChartObject_T, class Transform_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals, Transform_T transformer)
{
    QVector<DataType_T> values;
    const auto nSize = saturateCast<int>(Min(xVals.size(), yVals.size()));
    values.resize(nSize);
    // Copying from [x], [y] input to [(x,y)] storage that QCPStorage uses.
    transformer(values, xVals, yVals);
    fillQcpPlottable(rChartObject, std::move(values));
}

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals)
{
    using InputT = ::DFG_MODULE_NS(charts)::InputSpan<double>;
    const auto transformer = [](QVector<DataType_T>& dest, const InputT& xVals, const InputT& yVals)
    {
        std::transform(xVals.cbegin(), xVals.cbegin() + dest.size(), yVals.cbegin(), dest.begin(), [](const double x, const double y) { return DataType_T(x, y); });
    };
    fillQcpPlottable<DataType_T>(rChartObject, xVals, yVals, transformer);
}

static void fillQcpPlottable(QCPAbstractPlottable* pPlottable, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData)
{
    using namespace ::DFG_MODULE_NS(charts);
    if (!pPlottable)
        return;
    const auto px = pipeData.constValuesByIndex(0);
    const auto py = pipeData.constValuesByIndex(1);
    if (!px || !py)
        return;
    auto pGraph = qobject_cast<QCPGraph*>(pPlottable);
    if (pGraph)
    {
        fillQcpPlottable<QCPGraphData>(*pGraph, makeRange(*px), makeRange(*py));
        return;
    }
    auto pCurve = qobject_cast<QCPCurve*>(pPlottable);
    if (pCurve)
    {
        using InputT = ::DFG_MODULE_NS(charts)::InputSpan<double>;
        const auto transformer = [](QVector<QCPCurveData>& dest, const InputT& xVals, const InputT& yVals)
        {
            ::DFG_MODULE_NS(alg)::forEachFwdWithIndexT<size_t>(dest, [&](QCPCurveData& curveData, const size_t i)
                {
                    curveData = QCPCurveData(static_cast<double>(i), xVals[i], yVals[i]);
                });
        };
        fillQcpPlottable<QCPCurveData>(*pCurve, makeRange(*px), makeRange(*py), transformer);
        return;
    }
    auto pBars = qobject_cast<QCPBars*>(pPlottable);
    if (pBars)
    {
        fillQcpPlottable<QCPBarsData>(*pBars, makeRange(*px), makeRange(*py));
        return;
    }
    DFG_ASSERT_IMPLEMENTED(false);
}

template <class Func_T>
void forEachQCustomPlotLineStyle(const QCPGraph*, Func_T&& func)
{
    func(QCPGraph::lsNone, QT_TR_NOOP("None"));
    func(QCPGraph::lsLine, QT_TR_NOOP("Line"));
    func(QCPGraph::lsStepLeft, QT_TR_NOOP("Step, left-valued"));
    func(QCPGraph::lsStepRight, QT_TR_NOOP("Step, right-valued"));
    func(QCPGraph::lsStepCenter, QT_TR_NOOP("Step middle"));
    func(QCPGraph::lsImpulse, QT_TR_NOOP("Impulse"));
}

template <class Func_T>
void forEachQCustomPlotLineStyle(const QCPCurve*, Func_T&& func)
{
    func(QCPCurve::lsNone, QT_TR_NOOP("None"));
    func(QCPCurve::lsLine, QT_TR_NOOP("Line"));
}

template <class Func_T>
void forEachQCustomPlotScatterStyle(Func_T&& func)
{
    func(QCPScatterStyle::ssNone,               QT_TR_NOOP("None"));
    func(QCPScatterStyle::ssDot,                QT_TR_NOOP("Single pixel"));
    func(QCPScatterStyle::ssCross,              QT_TR_NOOP("Cross"));
    func(QCPScatterStyle::ssPlus,               QT_TR_NOOP("Plus"));
    func(QCPScatterStyle::ssCircle,             QT_TR_NOOP("Circle"));
    func(QCPScatterStyle::ssDisc,               QT_TR_NOOP("Disc"));
    func(QCPScatterStyle::ssSquare,             QT_TR_NOOP("Square"));
    func(QCPScatterStyle::ssDiamond,            QT_TR_NOOP("Diamond"));
    func(QCPScatterStyle::ssStar,               QT_TR_NOOP("Star"));
    func(QCPScatterStyle::ssTriangle,           QT_TR_NOOP("Triangle"));
    func(QCPScatterStyle::ssTriangleInverted,   QT_TR_NOOP("Inverted triangle"));
    func(QCPScatterStyle::ssCrossSquare,        QT_TR_NOOP("CrossSquare"));
    func(QCPScatterStyle::ssPlusSquare,         QT_TR_NOOP("PlusSquare"));
    func(QCPScatterStyle::ssCrossCircle,        QT_TR_NOOP("CrossCircle"));
    func(QCPScatterStyle::ssPlusCircle,         QT_TR_NOOP("PlusCircle"));
    func(QCPScatterStyle::ssPeace,              QT_TR_NOOP("Peace"));
}

// Wrapper to provide cbegin() et al that are missing from QCPDataContainer
template <class Cont_T>
class QCPContWrapper
{
public:
    using value_type = typename ::DFG_MODULE_NS(cont)::ElementType<typename std::remove_reference<typename std::remove_cv<decltype(Cont_T().begin())>::type>::type>::type;

    QCPContWrapper(Cont_T& ref) : m_r(ref) {}

    operator Cont_T& () { return m_r; }
    operator const Cont_T& () const { return m_r; }

    auto begin()        -> typename Cont_T::iterator { return m_r.begin(); }
    auto begin() const  -> typename Cont_T::const_iterator { return m_r.constBegin(); }
    auto cbegin() const -> typename Cont_T::const_iterator { return m_r.constBegin(); }

    auto end()        -> typename Cont_T::iterator { return m_r.end(); }
    auto end() const  -> typename Cont_T::const_iterator { return m_r.constEnd(); }
    auto cend() const -> typename Cont_T::const_iterator { return m_r.constEnd(); }

    int size() const { return m_r.size(); }

    Cont_T& m_r;
}; // class QCPContWrapper

template <class Cont_T, class PointToText_T>
static void createNearestPointToolTipList(Cont_T& cont, const PointXy& xy, ToolTipTextStream& toolTipStream, PointToText_T pointToText) // Note: QCPDataContainer doesn't seem to have const begin()/end() so must take cont by non-const reference.
{
    const auto nearestItems = ::DFG_MODULE_NS(alg)::nearestRangeInSorted(QCPContWrapper<Cont_T>(cont), xy.first, 5, [](const auto& dp) { return dp.key; });
    if (nearestItems.empty())
        return;

    toolTipStream << pointToText.tr("<br>Nearest by x-value:");
    // Adding items left of nearest
    for (const auto& dp : nearestItems.leftRange())
    {
        toolTipStream << QString("<br>%1").arg(pointToText(dp, toolTipStream));
    }
    // Adding nearest item in bold
    toolTipStream << QString("<br><b>%1</b>").arg(pointToText(*nearestItems.nearest(), toolTipStream));
    // Adding items right of nearest
    for (const auto& dp : nearestItems.rightRange())
    {
        toolTipStream << QString("<br>%1").arg(pointToText(dp, toolTipStream));
    }
}

static QString axisTypeToToolTipString(const QCPAxis::AxisType axisType)
{
    switch (axisType)
    {
    case QCPAxis::atLeft:  return QObject::tr("y");
    case QCPAxis::atRight: return QObject::tr("y2");
    default:               return QObject::tr("Unknown");
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
// 
//
/////////////////////////////////////////////////////////////////////////////////////


} } // dfg:::qt
