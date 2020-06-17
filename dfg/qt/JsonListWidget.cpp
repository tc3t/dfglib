#include "JsonListWidget.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QJsonDocument>
DFG_END_INCLUDE_QT_HEADERS

::DFG_MODULE_NS(qt)::JsonListParseError::JsonListParseError(int nRow, QJsonParseError jsonError)
    : m_nRow(nRow)
    , m_parseError(std::move(jsonError))
{
}

auto ::DFG_MODULE_NS(qt)::JsonListWidget::checkSyntax() const -> std::pair<bool, std::vector<JsonListParseError>>
{
    std::pair<bool, std::vector<JsonListParseError>> rv;
    rv.first = true;
    const auto items = entriesAsStringList();
    for (int r = 0, nCount = items.size(); r < nCount; ++r)
    {
        const auto& sJson = items[r];
        QJsonParseError parseError;
        auto parsed = QJsonDocument::fromJson(sJson.toUtf8(), &parseError);
        if (parsed.isNull()) // Parsing failed?
        {
            rv.first = false;
            rv.second.emplace_back(r, std::move(parseError));
        }
    }
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
