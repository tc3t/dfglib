
// graphtools implementations for QCustomPlot
#if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)

#include "../../charts/commonChartTools.hpp"
#include "../../charts/operations.hpp"
#include "../graphTools.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS

DFG_END_INCLUDE_QT_HEADERS

DFG_BEGIN_INCLUDE_WITH_DISABLED_WARNINGS
    #include "qcustomplot.h"
DFG_END_INCLUDE_WITH_DISABLED_WARNINGS

DFG_ROOT_NS_BEGIN{
    // QCPDataContainer doesn't seem to have value_type typedef, so adding specializations to ElementType<> so that QCPDataContainer works with nearestRangeInSorted()
    DFG_SUB_NS(cont)
    {
        namespace DFG_DETAIL_NS
        {
            template <> struct ElementTypeFromConstReferenceRemoved<QCPGraphDataContainer> { typedef QCPGraphData type; };
            template <> struct ElementTypeFromConstReferenceRemoved<QCPCurveDataContainer> { typedef QCPCurveData type; };
            template <> struct ElementTypeFromConstReferenceRemoved<QCPBarsDataContainer>  { typedef QCPBarsData type; };
        }
    } // module cont
} // namespace DFG_ROOT_NS


DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) {

template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, ValueCont_T&& yVals);

template <class DataType_T, class ChartObject_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, const ::DFG_MODULE_NS(charts)::InputSpan<double>& xVals, const ::DFG_MODULE_NS(charts)::InputSpan<double>& yVals);

static void fillQcpPlottable(QCPAbstractPlottable* pPlottable, ::DFG_MODULE_NS(charts)::ChartOperationPipeData& pipeData);

// Extended QPCurve providing metadata for every point; i.e. effectively a xy-graph with string info on third dimension shown in tooltip.
class CustomPlotCurveWithMetaData : public QCPCurve
{
public:
    using BaseClass = QCPCurve;
    using BaseClass::BaseClass;
    using PointMetaData = ::DFG_MODULE_NS(charts)::XySeries::PointMetaData;

    StringViewUtf8 metaDataStringAt(const QCPCurveData& data) const
    {
        return m_metaData[data.t].toStringView();
    }

    void setMetaDataStringAt(const QCPCurveData& data, const PointMetaData& metadata)
    {
        m_metaData.insert(data.t, metadata.string());
    }

    ::DFG_MODULE_NS(cont)::MapToStringViews<double, StringUtf8> m_metaData;
}; // class CustomPlotCurveWithMetaData

// Defines custom implementation for ChartObject. This is used to avoid repeating virtual overrides in ChartObjects.
class ChartObjectQCustomPlot : public ::DFG_MODULE_NS(charts)::ChartObject
{
public:
    using BaseClass = ::DFG_MODULE_NS(charts)::ChartObject;

    ChartObjectQCustomPlot(QCPAbstractPlottable* pPlottable)
        : BaseClass(::DFG_MODULE_NS(charts)::ChartObjectType::unknown)
        , m_spPlottable(pPlottable)
    {}
    virtual ~ChartObjectQCustomPlot() override {}

    QCPAbstractPlottable* qcpPlottable() { return m_spPlottable.data(); }
    const QCPAbstractPlottable* qcpPlottable() const { return m_spPlottable.data(); }

private:
    void setNameImpl(const ChartObjectStringView s) override;

    StringViewOrOwnerUtf8 nameImpl() const override;

    void setLineColourImpl(ChartObjectStringView svLineColour) override;

    void setFillColourImpl(ChartObjectStringView svFillColour) override;

    void setColourImpl(ChartObjectStringView svColour, std::function<void(QCPAbstractPlottable&, const QColor&)> setter);

    QPointer<QCPAbstractPlottable> m_spPlottable;
}; // class ChartObjectQCustomPlot

class XySeriesQCustomPlot : public ::DFG_MODULE_NS(charts)::XySeries
{
private:
    XySeriesQCustomPlot(QCPAbstractPlottable * pQcpObject);
public:
    XySeriesQCustomPlot(QCPGraph* pQcpGraph) : XySeriesQCustomPlot(static_cast<QCPAbstractPlottable*>(pQcpGraph)) {}
    XySeriesQCustomPlot(QCPCurve* pQcpCurve) : XySeriesQCustomPlot(static_cast<QCPAbstractPlottable*>(pQcpCurve)) {}

    template <class QCPObject_T, class Constructor_T>
    bool resizeImpl(QCPObject_T* pQcpObject, const DataSourceIndex nNewSize, Constructor_T constructor)
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

    void resize(const DataSourceIndex nNewSize) override
    {
        if (!resizeImpl(getGraph(), nNewSize, [](Dummy, double x, double y) { return QCPGraphData(x, y); }))
            resizeImpl(getCurve(), nNewSize, [](DataSourceIndex i, double x, double y) { return QCPCurveData(static_cast<double>(i), x, y); });
    }

    QCPGraph* getGraph()
    {
        return qobject_cast<QCPGraph*>(m_spQcpObject.data());
    }

    QCPCurve* getCurve()
    {
        return qobject_cast<QCPCurve*>(m_spQcpObject.data());
    }

    template <class DataType_T, class QcpObject_T, class DataContructor_T>
    bool setValuesImpl(QcpObject_T* pQcpObject, InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags, DataContructor_T constructor)
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

    void setValues(InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags) override
    {
        if (!setValuesImpl<QCPGraphData>(getGraph(), xVals, yVals, pFilterFlags, [](Dummy, double x, double y) { return QCPGraphData(x,y); }))
            setValuesImpl<QCPCurveData>(getCurve(), xVals, yVals, pFilterFlags, [](size_t n, double x, double y) { return QCPCurveData(static_cast<double>(n), x, y); });
    }

    size_t setMetaDataByFunctor(MetaDataSetterCallback func) override;

    void setLineStyle(StringViewC svStyle) override;

    void setPointStyle(StringViewC svStyle) override;

private:
    void setScatterStyle(const QCPScatterStyle style);
    // Unlike scatter style, graph and curve have distinct set of line styles, so handling them separately.
    bool setLineStyle(QCPGraph* pGraph, StringViewC svStyle); // Returns true iff non-null pGraph given.
    bool setLineStyle(QCPCurve* pCurve, StringViewC svStyle); // Returns true iff non-null pCurve given.

public:

    QPointer<QCPAbstractPlottable> m_spQcpObject;
}; // Class XySeriesQCustomPlot


class HistogramQCustomPlot : public ::DFG_MODULE_NS(charts)::Histogram
{
public:
    HistogramQCustomPlot(QCPBars * pBars);
    ~HistogramQCustomPlot() override;

    void setValues(InputSpanD xVals, InputSpanD yVals) override;

    // Returns effective bar width which is successful case is the same as given argument.
    double setBarWidth(double width);

    QPointer<QCPBars> m_spBars; // QCPBars is owned by QCustomPlot, not by *this.
}; // Class HistogramQCustomPlot


class BarSeriesQCustomPlot : public ::DFG_MODULE_NS(charts)::BarSeries
{
public:
    BarSeriesQCustomPlot(QCPBars * pBars);
    ~BarSeriesQCustomPlot() override;

    QPointer<QCPBars> m_spBars; // QCPBars is owned by QCustomPlot, not by *this.
}; // Class HistogramQCustomPlot

class ChartPanel;

using PointXy = ::DFG_MODULE_NS(cont)::TrivialPair<double, double>;

class ToolTipTextStream
{
public:
    ToolTipTextStream& operator<<(const QString& str)
    {
        m_sText += str;
        return *this;
    }

    const QString& toPlainText() const
    {
        return m_sText;
    }

    static QString numberToText(const double d)
    {
        return QString::number(d);
    }

    static QString numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker)
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

    static QString numberToText(const double d, QCPAxisTickerText* pTextTicker) // Note: pTextTicker param is not const because QCPAxisTickerText doesn't seem to provide const-way of asking ticks.
    {
        return (!pTextTicker) ? numberToText(d) : pTextTicker->ticks().value(d);
    }

    static QString numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker, QCPAxisTickerText* pTextTicker)
    {
        if (pTimeTicker)
            return numberToText(d, pTimeTicker);
        else if (pTextTicker)
            return numberToText(d, pTextTicker);
        else
            return numberToText(d);
    }

    bool isDestinationEmpty() const { return m_sText.isEmpty(); }

    QString m_sText;
}; // class ToolTipTextStream

class ChartCanvasQCustomPlot : public DFG_MODULE_NS(charts)::ChartCanvas, public QObject
{
public:
    using ChartEntryOperation = ::DFG_MODULE_NS(charts)::ChartEntryOperation;
    using ChartEntryOperationList = ::DFG_MODULE_NS(charts)::ChartEntryOperationList;
    using ChartObject = ::DFG_MODULE_NS(charts)::ChartObject;

    ChartCanvasQCustomPlot(QWidget* pParent = nullptr);

          QCustomPlot* getWidget() { return m_spChartView.get(); }
    const QCustomPlot* getWidget() const { return m_spChartView.get(); }

    void setPanelTitle(StringViewUtf8 svPanelId, StringViewUtf8 svTitle, StringViewUtf8 svColor) override;

    bool hasChartObjects() const override;

    void optimizeAllAxesRanges() override;

    void addContextMenuEntriesForChartObjects(void* pMenu) override;

    void removeAllChartObjects(bool bRepaint = true) override;

    void repaintCanvas() override;

    int width() const override;
    int height() const override;

    void setBackground(const StringViewUtf8&) override;

    void setPanelAxesColour(StringViewUtf8 svPanelId, StringViewUtf8 svColourDef) override;
    static void setPanelAxisColour(QCPAxis& axis, const QColor& color);
    static void resetPanelAxisColour(QCPAxis& axis);
    void setPanelAxesLabelColour(StringViewUtf8 svPanelId, StringViewUtf8 svColourDef) override;
    static void setPanelAxisLabelColour(QCPAxis& axis, const QColor& color);
    static void resetPanelAxisLabelColour(QCPAxis& axis);
    void setAxisProperties(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId, const ArgList& args) override;

    static void setTypeToQcpObjectProperty(QCPAbstractPlottable* pPlottable, const StringViewC& type);

    ChartObjectHolder<XySeries>  createXySeries(const XySeriesCreationParam& param)   override;
    ChartObjectHolder<Histogram> createHistogram(const HistogramCreationParam& param) override;
    std::vector<ChartObjectHolder<BarSeries>> createBarSeries(const BarSeriesCreationParam& param) override;

    void setAxisLabel(StringViewUtf8 sPanelId, StringViewUtf8 axisId, StringViewUtf8 axisLabel) override;
    void setAxisTickLabelDirection(StringViewUtf8 sPanelId, StringViewUtf8 axisId, StringViewUtf8 value) override;

    bool isLegendSupported() const override { return true; }
    bool isToolTipSupported() const override { return true; }
    bool isLegendEnabled() const override;
    bool isToolTipEnabled() const override { return m_bToolTipEnabled; }
    bool enableLegend(bool) override;
    bool enableToolTip(bool) override;
    void createLegends() override;

    // Implementation details
    QCPAxisRect* getAxisRect(const ChartObjectCreationParam& param);
    QCPAxis* getAxis(const ChartObjectCreationParam& param, QCPAxis::AxisType axisType);
    QCPAxis* getXAxis(const ChartObjectCreationParam& param);
    QCPAxis* getYAxis(const ChartObjectCreationParam& param);
    QCPAxisRect* getAxisRect(const StringViewUtf8& svPanelId);
    QCPAxis* getAxis(const StringViewUtf8& svPanelId, const StringViewUtf8& svAxisId);
    bool getGridPos(const StringViewUtf8 svPanelId, int& nRow, int& nCol);

    // Returns ChartPanel on which given (mouse) cursor is.
    ChartPanel* getChartPanel(const QPoint& pos); // pos is as available in mouseMoveEvent() pEvent->pos()
    ChartPanel* getChartPanel(const StringViewUtf8& svPanelId);
    ChartPanel* getChartPanelByAxis(const QCPAxis* pAxis); // Returns chart panel that has given axis, nullptr if not found.

    void removeLegends();

    void mouseMoveEvent(QMouseEvent* pEvent);

    void applyChartOperationsTo(QPointer<QCPAbstractPlottable> spPlottable);
    void applyChartOperationTo(QPointer<QCPAbstractPlottable> spPlottable, const StringViewUtf8& svOperationId);

    // Helper class for tooltip creation
    // Stores bars in a single visual bar stack.
    class BarStack
    {
    public:
        // Returns true iff this stack has given QCPBars.
        bool hasObject(const QCPBars& bars) const
        {
            for (const auto& info : m_subBarInfos)
            {
                if (info.m_spBars.data() == &bars)
                    return true;
            }
            return false;
        }

        static const QCPBars& findTopMostBar(const QCPBars& rBars)
        {
            auto pBars = &rBars;
            while (pBars->barAbove() != nullptr)
                pBars = pBars->barAbove();
            return *pBars;
        }

        void append(const QCPBarsData& data, const QCPBars& rBars)
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

        const QCPAxis* valueAxis() const;

        const QCustomPlot* parentPlot() const;

        void storeStackLabel(const QCPBarsData& data, const QCPBars& rBars);
        QString label() const;

        // Returns pointer to arbitrary QCPBars instance within this stack.
        const QCPBars* barsInstance() const;

        using SubBarHandler = std::function<void(const QString&, const QString&, const QColor&, bool)>;

        // Calls handler for every sub bar in order from top to bottom.
        void forEachSubBar(const PointXy& pointXy, ToolTipTextStream& toolTipStream, SubBarHandler) const;

        bool isXwithinStackX(const double x) const;

        double barWidth() const;

        size_t size() const { return m_subBarInfos.size(); }

        bool empty() const { return size() == 0; }

        class SubBarInfo
        {
        public:
            SubBarInfo(const QCPBarsData& data, const QCPBars& rBars)
            {
                m_spBars = &rBars;
                m_sLabel = rBars.name();
                m_data = data;
            }
            QPointer<const QCPBars> m_spBars;
            QString m_sLabel;
            QCPBarsData m_data;
        }; // class SubBarInfo

        QString m_sLabel; // Stack label
        double m_xValue = std::numeric_limits<double>::quiet_NaN();
        std::vector<SubBarInfo> m_subBarInfos; // 0 is topmost
    };

    // Helper class for tooltip creation
    // Stores set of BarStack objects
    class BarStacks
    {
    public:
        // Stores stack related to given object if applicable.
        // Note: this may invalidate pointers returned by findStacks()
        void storeStack(const QCPBars* pBars)
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
        std::vector<const BarStack*> findStacks(const QCPBars& bars) const
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
        bool hasBar(const QCPBars& bars) const
        {
            return !findStacks(bars).empty();
        }

        ::DFG_MODULE_NS(cont)::MapVectorSoA<double, BarStack> m_mapPosToBarStack;
    }; // BarStacks

    // Returns true if got non-null object as argument.
    static bool toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPCurve* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPBars* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const BarStack* pBarStack, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);

    template <class Func_T> static void forEachAxis(QCPAxisRect* pAxisRect, Func_T&& func);

    void beginUpdateState() override;

    ::DFG_MODULE_NS(charts)::ChartPanel* findPanelOfChartObject(const ChartObject*) override;

private:
    template <class This_T, class Func_T>
    static void forEachAxisRectImpl(This_T& rThis, Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func) const;

    template <class Func_T> void forEachChartPanelUntil(Func_T&& func); // Func shall return true to continue loop, false to break.

    // Returns true if operations were successfully created from given definition list, false otherwise.
    bool applyChartOperationsTo(QCPAbstractPlottable* pPlottable, const QStringList& definitionList);
    void applyChartOperationsTo(QCPAbstractPlottable* pPlottable, ChartEntryOperationList& operations);
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData createOperationPipeData(QCPAbstractPlottable* pPlottable);

public:

    QObjectStorage<QCustomPlot> m_spChartView;
    bool m_bLegendEnabled = false;
    bool m_bToolTipEnabled = true;
    QVector<QCPLegend*> m_legends; // All but the default legend
    QPointer<QCPItemText> m_spUpdateIndicator; // Note: owned by QCustomPlot-object.
}; // ChartCanvasQCustomPlot


class ChartPanel : public ::DFG_MODULE_NS(charts)::ChartPanel, public QCPLayoutGrid
{
    //Q_OBJECT
public:
    using PairT = PointXy;
    using AxisT = QCPAxis;
    ChartPanel(QCustomPlot* pQcp, StringViewUtf8 svPanelId);
          QCPAxisRect* axisRect();
    const QCPAxisRect* axisRect() const;

    void setTitle(StringViewUtf8 svTitle, StringViewUtf8 svColor = StringViewUtf8());
    QString getTitle() const;

    QString getPanelId() const { return m_panelId; }

    // Returns (x, y) pair from pixelToCoord() of primary axes.
    PairT pixelToCoord_primaryAxis(const QPoint& pos);

    void forEachChartObject(std::function<void(const QCPAbstractPlottable&)> handler);

          AxisT* primaryXaxis();
    const AxisT* primaryXaxis() const;
    AxisT* primaryYaxis();
    AxisT* secondaryXaxis();
    AxisT* secondaryYaxis();

    AxisT* axis(AxisT::AxisType axisType);
    const AxisT* axis(AxisT::AxisType axisType) const;

    bool hasAxis(const QCPAxis* pAxis);

    template <class Func_T> void forEachAxis(Func_T&& func);

    uint32 countOf(::DFG_MODULE_NS(charts)::AbstractChartControlItem::FieldIdStrViewInputParam type) const override;

private:
    template <class This_T>
    static auto axisImpl(This_T& rThis, AxisT::AxisType axisType) -> decltype(rThis.axis(axisType));

public:
    QCustomPlot* m_pQcp = nullptr;
    QString m_panelId;
}; // class ChartPanel


template <class This_T>
auto ChartPanel::axisImpl(This_T& rThis, AxisT::AxisType axisType) -> decltype(rThis.axis(axisType))
{
    auto pAxisRect = rThis.axisRect();
    return (pAxisRect) ? pAxisRect->axis(axisType) : nullptr;
}

template <class Func_T>
void ChartPanel::forEachAxis(Func_T&& func)
{
    auto pAxisRect = this->axisRect();
    if (!pAxisRect)
        return;
    ChartCanvasQCustomPlot::forEachAxis(pAxisRect, func);
}

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
}; // class GraphDisplayImageExportDialog

namespace
{
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
        func(QCPScatterStyle::ssNone, QT_TR_NOOP("None"));
        func(QCPScatterStyle::ssDot, QT_TR_NOOP("Single pixel"));
        func(QCPScatterStyle::ssCross, QT_TR_NOOP("Cross"));
        func(QCPScatterStyle::ssPlus, QT_TR_NOOP("Plus"));
        func(QCPScatterStyle::ssCircle, QT_TR_NOOP("Circle"));
        func(QCPScatterStyle::ssDisc, QT_TR_NOOP("Disc"));
        func(QCPScatterStyle::ssSquare, QT_TR_NOOP("Square"));
        func(QCPScatterStyle::ssDiamond, QT_TR_NOOP("Diamond"));
        func(QCPScatterStyle::ssStar, QT_TR_NOOP("Star"));
        func(QCPScatterStyle::ssTriangle, QT_TR_NOOP("Triangle"));
        func(QCPScatterStyle::ssTriangleInverted, QT_TR_NOOP("Inverted triangle"));
        func(QCPScatterStyle::ssCrossSquare, QT_TR_NOOP("CrossSquare"));
        func(QCPScatterStyle::ssPlusSquare, QT_TR_NOOP("PlusSquare"));
        func(QCPScatterStyle::ssCrossCircle, QT_TR_NOOP("CrossCircle"));
        func(QCPScatterStyle::ssPlusCircle, QT_TR_NOOP("PlusCircle"));
        func(QCPScatterStyle::ssPeace, QT_TR_NOOP("Peace"));
    }
} // unnamed namespace

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
} // unnamed namespace


namespace
{
    // Wrapper to provide cbegin() et al that are missing from QCPDataContainer
    template <class Cont_T>
    class QCPContWrapper
    {
    public:
        using value_type = typename ::DFG_MODULE_NS(cont)::ElementType<Cont_T>::type;

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
    };

    template <class Data_T>
    class PointToTextConverter
    {
    public:
        PointToTextConverter(const QCPAbstractPlottable& plottable)
        {
            const auto axisTicker = [](QCPAxis* pAxis) { return (pAxis) ? pAxis->ticker() : nullptr; };
            auto spXticker = axisTicker(plottable.keyAxis());
            auto spYticker = axisTicker(plottable.valueAxis());
            m_pXdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spXticker.data());
            m_pYdateTicker = dynamic_cast<const QCPAxisTickerDateTime*>(spYticker.data());
            m_pXtextTicker = dynamic_cast<QCPAxisTickerText*>(spXticker.data());
        }

        QString operator()(const Data_T& data, const ToolTipTextStream& toolTipStream) const
        {
            DFG_UNUSED(toolTipStream);
            return QString("(%1, %2)").arg(xText(data), yText(data));
        }

        QString xText(const Data_T& data) const
        {
            return ToolTipTextStream::numberToText(data.key, m_pXdateTicker, m_pXtextTicker);
        }

        QString yText(const Data_T& data) const
        {
            return ToolTipTextStream::numberToText(data.value, m_pYdateTicker);
        }

        static QString tr(const char* psz)
        {
            return QApplication::tr(psz);
        }

        const QCPAxisTickerDateTime* m_pXdateTicker = nullptr;
        const QCPAxisTickerDateTime* m_pYdateTicker = nullptr;
        QCPAxisTickerText* m_pXtextTicker = nullptr; // Not const because of reasons noted in ToolTipTextStream::numberToText()
    };

    template <class Cont_T, class PointToText_T>
    static void createNearestPointToolTipList(Cont_T& cont, const PointXy& xy, ToolTipTextStream& toolTipStream, PointToText_T pointToText) // Note: QCPDataContainer doesn't seem to have const begin()/end() so must take cont by non-const reference.
    {
        using DataT = typename ::DFG_MODULE_NS(cont)::ElementType<Cont_T>::type;
        const auto nearestItems = ::DFG_MODULE_NS(alg)::nearestRangeInSorted(QCPContWrapper<Cont_T>(cont), xy.first, 5, [](const DataT& dp) { return dp.key; });
        if (nearestItems.empty())
            return;

        toolTipStream << pointToText.tr("<br>Nearest by x-value:");
        // Adding items left of nearest
        for (const DataT& dp : nearestItems.leftRange())
        {
            toolTipStream << QString("<br>%1").arg(pointToText(dp, toolTipStream));
        }
        // Adding nearest item in bold
        toolTipStream << QString("<br><b>%1</b>").arg(pointToText(*nearestItems.nearest(), toolTipStream));
        // Adding items right of nearest
        for (const DataT& dp : nearestItems.rightRange())
        {
            toolTipStream << QString("<br>%1").arg(pointToText(dp, toolTipStream));
        }
    }

    QString axisTypeToToolTipString(const QCPAxis::AxisType axisType)
    {
        switch (axisType)
        {
        case QCPAxis::atLeft:  return QObject::tr("y");
        case QCPAxis::atRight: return QObject::tr("y2");
        default:               return QObject::tr("Unknown");
        }
    }

} // unnamed namespace

template <class ChartObject_T, class ValueCont_T>
static void fillQcpPlottable(ChartObject_T& rChartObject, ValueCont_T&& data)
{
    // Note the terminology (using QCPGraph as example)
    //  QCPGraph().data() == QSharedPointer<QCPGraphDataContainer> -> DataContainer == QCPGraphDataContainer == QCPDataContainer<QCPGraphData>
    //  QCPGraph().data()->set() takes QVector<QCPGraphData> which is the actual storage, this is ValueCont_T
    using DataContainer = typename decltype(rChartObject.data())::value_type;
    QSharedPointer<DataContainer> spData(new DataContainer);
    std::sort(data.begin(), data.end(), qcpLessThanSortKey<typename ValueCont_T::value_type>);
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


} } // dfg:::qt

#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
