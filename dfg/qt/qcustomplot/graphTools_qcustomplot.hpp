
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

// QCPDataContainer doesn't seem to have value_type typedef, so adding specializations to ElementType<> so that QCPDataContainer works with nearestRangeInSorted()
DFG_ROOT_NS_BEGIN
{
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

    void resize(const DataSourceIndex nNewSize) override;

    QCPGraph* getGraph();
    QCPCurve* getCurve();

    void setValues(InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags) override;

    size_t setMetaDataByFunctor(MetaDataSetterCallback func) override;

    void setLineStyle(StringViewC svStyle) override;

    void setPointStyle(StringViewC svStyle) override;

private:
    template <class QCPObject_T, class Constructor_T>
    bool resizeImpl(QCPObject_T* pQcpObject, const DataSourceIndex nNewSize, Constructor_T constructor);

    template <class DataType_T, class QcpObject_T, class DataContructor_T>
    bool setValuesImpl(QcpObject_T* pQcpObject, InputSpanD xVals, InputSpanD yVals, const std::vector<bool>* pFilterFlags, DataContructor_T constructor);

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

class StatisticalBoxQCustomPlot : public ::DFG_MODULE_NS(charts)::StatisticalBox
{
public:
    StatisticalBoxQCustomPlot(QCPStatisticalBox* pStatBox);
    ~StatisticalBoxQCustomPlot() override;

    QPointer<QCPStatisticalBox> m_spStatBox; // QCPStatisticalBox is owned by QCustomPlot, not by *this. (https://www.qcustomplot.com/documentation/classQCPStatisticalBox.html)
}; // Class StatisticalBoxQCustomPlot

class ChartPanel;

using PointXy = ::DFG_MODULE_NS(cont)::TrivialPair<double, double>;

class ToolTipTextStream
{
public:
    ToolTipTextStream& operator<<(const QString& str);

    const QString& toPlainText() const;

    static QString numberToText(const double d);

    static QString numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker);

    static QString numberToText(const double d, QCPAxisTickerText* pTextTicker); // Note: pTextTicker param is not const because QCPAxisTickerText doesn't seem to provide const-way of asking ticks.

    static QString numberToText(const double d, const QCPAxisTickerDateTime* pTimeTicker, QCPAxisTickerText* pTextTicker);

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

          QCustomPlot* getWidget()       { return m_spChartView.get(); }
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
    std::vector<ChartObjectHolder<StatisticalBox>> createStatisticalBox(const StatisticalBoxCreationParam& param) override;

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
    const ChartPanel* getChartPanel(const QPoint& pos) const; // const overload
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
        bool hasObject(const QCPBars& bars) const;

        static const QCPBars& findTopMostBar(const QCPBars& rBars);

        void append(const QCPBarsData& data, const QCPBars& rBars);

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
    }; // class BarStack

    // Helper class for tooltip creation
    // Stores set of BarStack objects
    class BarStacks
    {
    public:
        // Stores stack related to given object if applicable.
        // Note: this may invalidate pointers returned by findStacks()
        void storeStack(const QCPBars* pBars);

        // Returns BarStacks related to given QCPBars-object
        std::vector<const BarStack*> findStacks(const QCPBars& bars) const;

        // Returns true iff given QCPBars is within any stored stack.
        bool hasBar(const QCPBars& bars) const;

        ::DFG_MODULE_NS(cont)::MapVectorSoA<double, BarStack> m_mapPosToBarStack;
    }; // BarStacks

    // Returns true if got non-null object as argument.
    static bool toolTipTextForChartObjectAsHtml(const QCPGraph* pGraph, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    bool toolTipTextForChartObjectAsHtml(const QCPCurve* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream) const;
    static bool toolTipTextForChartObjectAsHtml(const QCPBars* pBars, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const BarStack* pBarStack, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);
    static bool toolTipTextForChartObjectAsHtml(const QCPStatisticalBox* pStatBox, const PointXy& cursorXy, ToolTipTextStream& toolTipStream);

    template <class Func_T> static void forEachAxis(QCPAxisRect* pAxisRect, Func_T&& func);

    void beginUpdateState() override;

    ::DFG_MODULE_NS(charts)::ChartPanel* findPanelOfChartObject(const ChartObject*) override;

private:
    template <class This_T, class Func_T>
    static void forEachAxisRectImpl(This_T& rThis, Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func);
    template <class Func_T> void forEachAxisRect(Func_T&& func) const;

    template <class Func_T> void forEachChartPanelUntil(Func_T&& func); // Func shall return true to continue loop, false to break.
    template <class Func_T> void forEachChartPanelUntil(Func_T&& func) const; // Const overload
    template <class This_T, class Func_T>
    static void forEachChartPanelUntil(This_T& rThis, Func_T&& func);

    template <class This_T>
    static auto getChartPanelImpl(This_T& rThis, const QPoint& pos) -> std::conditional_t<std::is_const_v<This_T>, const ChartPanel*, ChartPanel*>;

    // Returns true if operations were successfully created from given definition list, false otherwise.
    bool applyChartOperationsTo(QCPAbstractPlottable* pPlottable, const QStringList& definitionList);
    void applyChartOperationsTo(QCPAbstractPlottable* pPlottable, ChartEntryOperationList& operations);
    ::DFG_MODULE_NS(charts)::ChartOperationPipeData createOperationPipeData(QCPAbstractPlottable* pPlottable);

    // Private virtual overrides -->
    StringUtf8 createToolTipTextAtImpl(int x, int y, ToolTipTextRequestFlags flags) const override;
    // <-- private virtual overrides
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
    PairT pixelToCoord_primaryAxis(const QPoint& pos) const;

    void forEachChartObject(std::function<void(const QCPAbstractPlottable&)> handler) const;

          AxisT* primaryXaxis();
    const AxisT* primaryXaxis() const;
          AxisT* primaryYaxis();
    const AxisT* primaryYaxis() const;
    AxisT* secondaryXaxis();
    AxisT* secondaryYaxis();

    AxisT* axis(AxisT::AxisType axisType);
    const AxisT* axis(AxisT::AxisType axisType) const;

    bool hasAxis(const QCPAxis* pAxis);

    template <class Func_T> void forEachAxis(Func_T&& func);

    uint32 countOf(::DFG_MODULE_NS(charts)::AbstractChartControlItem::FieldIdStrViewInputParam type) const override;

    // Finds existing object in panel with given type and if found, return one. If there are multiple, it's unspecified which one is returned.
    QCPAbstractPlottable* findExistingOfType(::DFG_MODULE_NS(charts)::AbstractChartControlItem::FieldIdStrViewInputParam type) const;

private:
    template <class This_T>
    static auto axisImpl(This_T& rThis, AxisT::AxisType axisType) -> decltype(rThis.axis(axisType));

public:
    QCustomPlot* m_pQcp = nullptr;
    QString m_panelId;
}; // class ChartPanel

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

} } // dfg:::qt

#endif // #if defined(DFG_ALLOW_QCUSTOMPLOT) && (DFG_ALLOW_QCUSTOMPLOT == 1)
