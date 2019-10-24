#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include "containerUtils.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QWidget>
    #include <QFrame>
DFG_END_INCLUDE_QT_HEADERS


DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

// Widget that shows graph display, no controls.
class GraphDisplay : public QWidget
{
    Q_OBJECT
public:
    typedef QWidget BaseClass;

    GraphDisplay(QWidget* pParent);
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

    QObjectStorage<GraphControlPanel> m_spControlPanel;

}; // Class GraphControlAndDisplayWidget

} } // module namespace
