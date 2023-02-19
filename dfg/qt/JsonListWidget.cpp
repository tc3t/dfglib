#include "JsonListWidget.hpp"
#include "widgetHelpers.hpp"
#include "connectHelper.hpp"
#include "PropertyHelper.hpp"
#include "../dfgBase.hpp"
#include "../alg.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QInputDialog>
    #include <QJsonDocument>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::JsonListParseError::JsonListParseError(int nRow, QJsonParseError jsonError)
    : m_nRow(nRow)
    , m_parseError(std::move(jsonError))
{
}

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace DFG_DETAIL_NS
{
    enum class JsonListWidgetPropertyId
    {
         // Add properties here in the beginning (or remember to update JsonListWidgetPropertyId_last)
        lineWrapping,
        fontPointSize,
        last = fontPointSize
     };

    // Properties
    DFG_QT_DEFINE_OBJECT_PROPERTY_CLASS(JsonListWidget)
    DFG_QT_DEFINE_OBJECT_PROPERTY("JsonListWidget_fontPointSize",
        JsonListWidget,
        JsonListWidgetPropertyId::fontPointSize,
        double,
        []() { return 10.0; } ); // Default font seemed to be 8 at least in Qt 5.13 & MSVC2017, a bit small.
    DFG_QT_DEFINE_OBJECT_PROPERTY("JsonListWidget_lineWrapping",
        JsonListWidget,
        JsonListWidgetPropertyId::lineWrapping,
        bool,
        []() { return true; });

    template <JsonListWidgetPropertyId ID>
    auto getJsonListWidgetProperty(JsonListWidget* widget) -> typename DFG_QT_OBJECT_PROPERTY_CLASS_NAME(JsonListWidget)<static_cast<int>(ID)> ::PropertyType
    {
        return DFG_MODULE_NS(qt)::getProperty<DFG_QT_OBJECT_PROPERTY_CLASS_NAME(JsonListWidget)<static_cast<int>(ID)>>(widget);
    }

}}} // namespace dfg::qt::DFG_DETAIL_NS

DFG_MODULE_NS(qt)::JsonListWidget::JsonListWidget(QWidget* pParent)
    : BaseClass(pParent)
{
    const double fontPointSize = DFG_DETAIL_NS::getJsonListWidgetProperty<DFG_DETAIL_NS::JsonListWidgetPropertyId::fontPointSize>(this);
    adjustWidgetFontProperties(this, fontPointSize);
    const auto bLineWrapping = DFG_DETAIL_NS::getJsonListWidgetProperty<DFG_DETAIL_NS::JsonListWidgetPropertyId::lineWrapping>(this);
    this->setLineWrapping(bLineWrapping);
}

::DFG_MODULE_NS(qt)::JsonListWidget::~JsonListWidget() = default;

void ::DFG_MODULE_NS(qt)::JsonListWidget::contextMenuEvent(QContextMenuEvent* pEvent)
{
    if (!pEvent)
        return;

    // Taking standard context menu...
    std::unique_ptr<QMenu> spMenu(BaseClass::createStandardContextMenu(pEvent->pos())); // Not sure if pos is given in correct units (global vs widget)

    // ...and adding separator before custom entries.
    spMenu->addSeparator();

    // 'Set font size'-item
    {
        auto pAction = spMenu->addAction(tr("Set font size"));
        if (pAction)
        {
            const auto nCurrentFontPointSize = this->font().pointSize();
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, [=]()
            {
                const auto nNewSize = QInputDialog::getInt(this, tr("New font size"), QString(), nCurrentFontPointSize);
                if (nNewSize != nCurrentFontPointSize && nNewSize > 0 && nNewSize < 1000)
                    adjustWidgetFontProperties(this, nNewSize);
            }));
        }
    }

    // 'Line wrapping'-item
    {
        auto pAction = spMenu->addAction(tr("Line wrapping"));
        if (pAction)
        {
            pAction->setCheckable(true);
            pAction->setChecked(this->lineWrapMode() != QPlainTextEdit::NoWrap);
            DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &JsonListWidget::setLineWrapping));
        }
    }

    // Content manipulation actions
    {
        // Comment-action
        {
            auto pAction = spMenu->addAction(tr("Add comment to selection"));
            if (pAction)
                DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &JsonListWidget::addCommentToSelection));
        }
        // Uncomment-action
        {
            auto pAction = spMenu->addAction(tr("Remove comment from selection"));
            if (pAction)
                DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &JsonListWidget::removeCommentFromSelection));
        }
        // Escape for json-action
        {
            auto pAction = spMenu->addAction(tr("Escape selection for json-field"));
            if (pAction)
                DFG_QT_VERIFY_CONNECT(connect(pAction, &QAction::triggered, this, &JsonListWidget::escapeSelectionForJsonField));
        }
    }

    // Actions from 'this'
    {
        spMenu->addSeparator();
        spMenu->addActions(this->actions());
    }

    spMenu->move(this->mapToGlobal(pEvent->pos()));
    spMenu->exec();
}

void ::DFG_MODULE_NS(qt)::JsonListWidget::setLineWrapping(const bool bWrap)
{
    this->setLineWrapMode((bWrap) ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void ::DFG_MODULE_NS(qt)::JsonListWidget::addCommentToSelection()
{
    setSelectionCommenting(true);
}

void ::DFG_MODULE_NS(qt)::JsonListWidget::removeCommentFromSelection()
{
    setSelectionCommenting(false);
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::escapeStringToJsonField(const QString& sInput) -> QString
{
    // Based on https://stackoverflow.com/a/33799784 ('Simple JSON string escape for C++?')
    QString sOutput;
    for (const auto& c : sInput)
    {
        switch (c.unicode())
        {
            case '"' : sOutput += "\\\""; break;
            case '\\': sOutput += "\\\\"; break;
            case '\b': sOutput += "\\b";  break;
            case '\f': sOutput += "\\f";  break;
            case '\n': sOutput += "\\n";  break;
            case '\r': sOutput += "\\r";  break;
            case '\t': sOutput += "\\t";  break;
            default: sOutput += ('\x00' <= c && c <= '\x1f') ? QString("\\u%1").arg(c.unicode(), 4, 10, QChar('0')) : c; break;
        }
    }
    return sOutput;
}

void ::DFG_MODULE_NS(qt)::JsonListWidget::escapeSelectionForJsonField()
{
    auto [lines, cursor] = getLinesInSelection();
    if (lines.size() == 0)
        return;

    for (auto& s : lines)
        s = escapeStringToJsonField(s);

    cursor.insertText(lines.join('\n'));
}

void ::DFG_MODULE_NS(qt)::JsonListWidget::setSelectionCommenting(const bool bComment)
{
    auto [lines, cursor] = getLinesInSelection();
    for (auto& s : lines)
    {
        if (bComment)
            s.prepend('#');
        else if (!s.isEmpty() && s[0] == '#')
            s.remove(0, 1);
    }
    cursor.insertText(lines.join('\n'));
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::getLinesInSelection() -> std::pair<QStringList, QTextCursor>
{
    auto cursor = this->textCursor();
    const auto sSelectedText = cursor.selectedText();
    auto lines = sSelectedText.split(QChar(8233)); // If there are line breaks in selection, selectedText() will "contain a Unicode U+2029 paragraph separator character instead of a newline \n character" (Qt 5.13 documentation)
    return { lines, cursor };
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::checkSyntax() const -> std::pair<bool, std::vector<JsonListParseError>>
{
    std::pair<bool, std::vector<JsonListParseError>> rv;
    rv.first = true;
    const auto items = entriesAsStringList();
    ::DFG_MODULE_NS(alg)::forEachFwdWithIndexT<int>(items, [&](const QString& sJson, const int r)
    {
        QJsonParseError parseError;
        auto parsed = QJsonDocument::fromJson(sJson.toUtf8(), &parseError);
        if (parsed.isNull()) // Parsing failed?
        {
            rv.first = false;
            rv.second.emplace_back(r, std::move(parseError));
        }
    });
    return rv;
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::formatErrorMessage(const SyntaxCheckResult& syntaxCheckResult) -> QString
{
    if (syntaxCheckResult.first)
        return tr("ok");
    QString rv;
    for (const auto& item : syntaxCheckResult.second)
    {
        if (!rv.isEmpty())
            rv += "\n";
        rv += tr("Line %1, offset %2: %3").arg(item.m_nRow).arg(item.m_parseError.offset).arg(item.m_parseError.errorString());
    }
    return rv;
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::entriesAsStringList() const -> QStringList
{
    auto items = this->toPlainText().split('\n');
    const auto isEmptyOrCommented = [](const QString& s) { return s.isEmpty() || s[0] == '#'; };
    items.erase(std::remove_if(items.begin(), items.end(), isEmptyOrCommented), items.end());
    return items;
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::entriesAsNewLineSeparatedString() const -> QString
{
    return entriesAsStringList().join('\n');
}
