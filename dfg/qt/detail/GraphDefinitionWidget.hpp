#pragma once

#include <QWidget>
#include <QElapsedTimer>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
//   GraphDefinitionWidget
//
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

// Defines UI for controlling chart entries
// Requirements:
//      -Must provide text-based export/import of settings in order to easily store/restore previous settings.
class GraphDefinitionWidget : public QWidget
{
public:
    typedef QWidget BaseClass;

    GraphDefinitionWidget(GraphControlPanel* pNonNullParent);

    QString getRawTextDefinition() const;

    void addJsonWidgetContextMenuEntries(JsonListWidget& rJsonListWidget);

    void showGuideWidget();

    static QString getGuideString();
    void setGuideString(const QString& s);

    ChartDefinition getChartDefinition();

    ChartDefinitionViewer getChartDefinitionViewer();

    ChartController* getController();

    void onTextDefinitionChanged();

    void updateChartDefinitionViewable();
    void checkUpdateChartDefinitionViewableTimer();

    void onActionButtonClicked();

    QObjectStorage<JsonListWidget> m_spRawTextDefinition; // Guaranteed to be non-null between constructor and destructor.
    QObjectStorage<QWidget> m_spGuideWidget;
    QObjectStorage<QPushButton> m_spApplyButton;
    QPointer<GraphControlPanel> m_spParent;
    ChartDefinitionViewable m_chartDefinitionViewable;
    QElapsedTimer m_timeSinceLastEdit;
    QTimer m_chartDefinitionTimer;
    const int m_nChartDefinitionUpdateMinimumIntervalInMs = 250;

    static QString s_sGuideString;
    static const char s_szApplyText[];
    static const char s_szTerminateText[];
}; // Class GraphDefinitionWidget

}} // namespace dfg::qt
