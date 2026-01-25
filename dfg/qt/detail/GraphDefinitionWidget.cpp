#include "GraphDefinitionWidget.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

QString GraphDefinitionWidget::s_sGuideString;
const char GraphDefinitionWidget::s_szApplyText[] = QT_TR_NOOP("Apply");
const char GraphDefinitionWidget::s_szTerminateText[] = QT_TR_NOOP("Terminate");

GraphDefinitionWidget::GraphDefinitionWidget(GraphControlPanel * pNonNullParent) :
    BaseClass(pNonNullParent),
    m_spParent(pNonNullParent)
{
    DFG_ASSERT(m_spParent.data() != nullptr);

    auto spLayout = std::make_unique<QVBoxLayout>(this);

    m_spRawTextDefinition.reset(new JsonListWidget(this));
    m_spRawTextDefinition->setPlainText(GraphDefinitionEntry::xyGraph(QString(), QString()).toText());
    addJsonWidgetContextMenuEntries(*m_spRawTextDefinition);
    DFG_QT_VERIFY_CONNECT(connect(m_spRawTextDefinition.get(), &JsonListWidget::textChanged, this, &GraphDefinitionWidget::onTextDefinitionChanged));

    spLayout->addWidget(m_spRawTextDefinition.get());

    // Adding control buttons
    {
        auto spHlayout = std::make_unique<QHBoxLayout>();
        m_spApplyButton.reset(new QPushButton(tr(s_szApplyText), this));
        auto pGuideButton = new QPushButton(tr("Show guide"), this); // Deletion through parentship.
        pGuideButton->setObjectName("guideButton");
        pGuideButton->setHidden(true);

        DFG_QT_VERIFY_CONNECT(connect(m_spApplyButton.get(), &QPushButton::clicked, this, &GraphDefinitionWidget::onActionButtonClicked));
        DFG_QT_VERIFY_CONNECT(connect(pGuideButton, &QPushButton::clicked, this, &GraphDefinitionWidget::showGuideWidget));

        spHlayout->addWidget(m_spApplyButton.get());
        spHlayout->addWidget(pGuideButton);
        spLayout->addLayout(spHlayout.release()); // spHlayout is made child of spLayout.
    }

    setLayout(spLayout.release()); // Ownership transferred to *this.

    // Setting m_chartDefinitionTimer
    {
        DFG_QT_VERIFY_CONNECT(connect(&m_chartDefinitionTimer, &QTimer::timeout, this, &GraphDefinitionWidget::checkUpdateChartDefinitionViewableTimer));
        m_chartDefinitionTimer.setSingleShot(true);
    }
}

void GraphDefinitionWidget::addJsonWidgetContextMenuEntries(JsonListWidget & rJsonListWidget)
{
    // 'Insert'-items
    auto pJsonListWidget = &rJsonListWidget;
    {
        const auto addInsertAction = [&](const QString& sTitle, const QString& sInsertText)
        {
            std::unique_ptr<QAction> spAction(new QAction(sTitle, &rJsonListWidget));

            const auto handler = [=]()
            {
                pJsonListWidget->insertPlainText(sInsertText);
            };
            DFG_QT_VERIFY_CONNECT(connect(spAction.get(), &QAction::triggered, handler));
            rJsonListWidget.addAction(spAction.release()); // Deletion through parentship.
        };
        addInsertAction(tr("Insert basic 'xy'"),        R"({ "type":"xy" })");
        addInsertAction(tr("Insert basic 'txy'"),       R"({ "type":"txy" })");
        addInsertAction(tr("Insert basic 'txys'"),      R"({ "type":"txys" })");
        addInsertAction(tr("Insert basic 'bars'"),      R"({ "type":"bars" })");
        addInsertAction(tr("Insert basic 'histogram'"), R"({ "type":"histogram" })");
        addInsertAction(tr("Insert basic 'stat_box'"),  R"({ "type":"stat_box" })");
        addInsertAction(tr("Insert basic 'panel_config'"),
            R"({"type": "panel_config", "title": "Panel title", "x_label": "x label", "y_label": "y label"})");
        addInsertAction(tr("Insert basic 'global_config'"),
            R"({ "type":"global_config", "show_legend":true })");
    }
}

void GraphDefinitionWidget::onTextDefinitionChanged()
{
    m_timeSinceLastEdit.start();
    if (!m_chartDefinitionTimer.isActive())
        m_chartDefinitionTimer.start(m_nChartDefinitionUpdateMinimumIntervalInMs);
}

void GraphDefinitionWidget::updateChartDefinitionViewable()
{
    auto newChartDef = getChartDefinition();
    m_chartDefinitionViewable.edit([&](ChartDefinition& cd, const ChartDefinition* pOld)
    {
        DFG_UNUSED(pOld);
        cd = std::move(newChartDef);
    });
}

void GraphDefinitionWidget::checkUpdateChartDefinitionViewableTimer()
{
    if (!m_timeSinceLastEdit.isValid())
        return;
    const auto elapsed = m_timeSinceLastEdit.elapsed();
    const auto nEffectiveLimit = m_nChartDefinitionUpdateMinimumIntervalInMs * 19 / 20; // Allowing some inaccuracies (5 %) so that if this function gets called e.g. at 249 ms with limit 250 ms, interpreting as close enough.
    if (elapsed >= nEffectiveLimit)
        updateChartDefinitionViewable();
    else // case: not time to update yet, scheduling new check.
        m_chartDefinitionTimer.start(m_nChartDefinitionUpdateMinimumIntervalInMs - static_cast<int>(elapsed));
}

void GraphDefinitionWidget::setGuideString(const QString & s)
{
    if (s_sGuideString.isEmpty())
    {
        s_sGuideString = s;
        auto pGuideButton = this->findChild<QPushButton*>("guideButton");
        if (pGuideButton)
            pGuideButton->setHidden(false);
    }
    else
    {
        DFG_ASSERT_CORRECTNESS(false); // Guide string can be set only once.
    }
}

QString GraphDefinitionWidget::getGuideString()
{
    // Originally guide string was embedded - while waiting for std::embed (http://open-std.org/JTC1/SC22/WG21/docs/papers/2020/p1040r6.html) -  here like this:
    //return QString(
    //#include "res/chartGuide.html"
    //);
    // The file, however, was starting to get too big (~16 kB for MSVC2017) for this pattern causing compile error.
    // And while this is in Qt code, using qrc resources seemed tricky as being part of library, 
    // there's no executable or compiled library to embed to without introducing additional build requirements.
    // So the solution for now is to require the guide to be set from outside, where it hopefully is easier to embed e.g. as qrc-resource.
    return s_sGuideString;
}

void GraphDefinitionWidget::showGuideWidget()
{
    if (!m_spGuideWidget)
    {
        m_spGuideWidget.reset(new QDialog(this));
        m_spGuideWidget->setWindowTitle(tr("Chart guide"));
        auto pTextEdit = new QTextEdit(m_spGuideWidget.get()); // Deletion through parentship.
        auto pLayout = new QVBoxLayout(m_spGuideWidget.get());
        pTextEdit->setTextInteractionFlags(Qt::TextBrowserInteraction);
        pTextEdit->setHtml(getGuideString());

        // At least on Qt 5.13 with MSVC2017 default font size was a bit small (8.25), increasing to 11.
        {
            auto font = pTextEdit->currentFont();
            const auto oldPointSizeF = font.pointSizeF();
            font.setPointSizeF(Max(qreal(11), oldPointSizeF));
            pTextEdit->setFont(font);
        }

        // Creating edit for find text and adding it to layout
        auto pFindEdit = new QLineEdit(m_spGuideWidget.get()); // Deletion through parentship.
        pFindEdit->setMaxLength(1000);
        pFindEdit->setPlaceholderText("Find... (Ctrl+F)");
        pLayout->addWidget(pFindEdit);

        // Adjusting colors in text edit so that highlighted text shows more clearly even if the text edit does not have focus.
        {
            QPalette pal = pTextEdit->palette();
            pal.setColor(QPalette::Inactive, QPalette::Highlight, pal.color(QPalette::Active, QPalette::Highlight));
            pal.setColor(QPalette::Inactive, QPalette::HighlightedText, pal.color(QPalette::Active, QPalette::HighlightedText));
            pTextEdit->setPalette(pal);
        }

        // Adding shortcut Ctrl+F (activate find edit)
        {
            auto pActivateFind = new QAction(pTextEdit);
            pActivateFind->setShortcut(QString("Ctrl+F"));
            m_spGuideWidget->addAction(pActivateFind); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActivateFind, &QAction::triggered, pFindEdit, [=]() { pFindEdit->setFocus(); pFindEdit->selectAll(); }));
        }

        const auto wrapCursor = [](QTextEdit* pTextEdit, QLineEdit* pFindEdit, const QTextDocument::FindFlags findFlags, const QTextCursor::MoveOperation moveOp)
            {
                if (!pTextEdit || !pFindEdit)
                    return;
                if (!pTextEdit->find(pFindEdit->text(), findFlags)) // If didn't find, wrapping to begin/end and trying to find again.
                {
                    const auto initialCursor = pTextEdit->textCursor();
                    pTextEdit->moveCursor(moveOp);
                    if (!pTextEdit->find(pFindEdit->text(), findFlags))
                        pTextEdit->setTextCursor(initialCursor); // Didn't find after wrapping so restoring cursor position.
                }
            };

        // Adding shortcut F3 (find)
        {
            auto pActionF3 = new QAction(m_spGuideWidget.get());
            pActionF3->setShortcut(QString("F3"));
            m_spGuideWidget->addAction(pActionF3); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActionF3, &QAction::triggered, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindFlags(), QTextCursor::Start); }));
        }

        // Adding shortcut Shift+F3 (find backward)
        {
            auto pActionShiftF3 = new QAction(m_spGuideWidget.get());
            pActionShiftF3->setShortcut(QString("Shift+F3"));
            m_spGuideWidget->addAction(pActionShiftF3); // Needed in order to get shortcut trigger; parentship is not enough.
            DFG_QT_VERIFY_CONNECT(connect(pActionShiftF3, &QAction::triggered, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindBackward, QTextCursor::End); }));
        }

        // Connecting textChanged() to trigger find.
        DFG_QT_VERIFY_CONNECT(connect(pFindEdit, &QLineEdit::textChanged, pTextEdit, [=]() { wrapCursor(pTextEdit, pFindEdit, QTextDocument::FindFlags(), QTextCursor::Start); }));

        pLayout->addWidget(pTextEdit);
        removeContextHelpButtonFromDialog(m_spGuideWidget.get());

        // Resizing guide window to 75 % of mainwindow size (if mainwindow is available) or at least (600, 500)
        {
            auto pMainWindow = getWidgetByType<QMainWindow>(QApplication::topLevelWidgets());
            const auto width = Max(600, (pMainWindow) ? 3 * pMainWindow->width() / 4 : 0);
            const auto height = Max(500, (pMainWindow) ? 3 * pMainWindow->height() / 4 : 0);
            m_spGuideWidget->resize(width, height);
        }
    }
    m_spGuideWidget->show();
}

auto GraphDefinitionWidget::getChartDefinition() -> ChartDefinition
{
    auto pController = getController();
    auto defaultSource = (pController) ? pController->defaultSourceId() : GraphDataSourceId();
    return ChartDefinition((m_spRawTextDefinition) ? m_spRawTextDefinition->toPlainText() : QString(), defaultSource);
}

auto GraphDefinitionWidget::getChartDefinitionViewer() -> ChartDefinitionViewer
{
    this->updateChartDefinitionViewable();
    return m_chartDefinitionViewable.createViewer();
}

QString GraphDefinitionWidget::getRawTextDefinition() const
{
    return (m_spRawTextDefinition) ? m_spRawTextDefinition->toPlainText() : QString();
}

ChartController* GraphDefinitionWidget::getController()
{
    return (m_spParent) ? m_spParent->getController() : nullptr;
}

void GraphDefinitionWidget::onActionButtonClicked()
{
    auto pButton = m_spApplyButton.get();
    if (!pButton)
        return;

    auto pController = getController();
    if (!pController)
        return;

    const auto sButtonText = pButton->text();
    if (sButtonText == tr(s_szApplyText))
        pController->refresh();
    else if (sButtonText.startsWith(tr(s_szTerminateText)))
    {
        pController->refreshTerminate();
        pButton->setText("Terminating...");
        pButton->setEnabled(false);
    }
}

} }
