#pragma once

#include "../dfgDefs.hpp"
#include "qtIncludeHelpers.hpp"
#include <vector>

DFG_BEGIN_INCLUDE_QT_HEADERS
#include <QJsonParseError>
#include <QPlainTextEdit>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(qt)
{

class JsonListParseError
{
public:
    JsonListParseError(int nRow, QJsonParseError jsonError);

    int m_nRow = 0;
    QJsonParseError m_parseError;
};

// Provides UI for viewing and controlling list of (single line) JSON items.
class JsonListWidget : public QPlainTextEdit
{
public:
    using BaseClass = QPlainTextEdit;
    using SyntaxCheckResult = std::pair<bool, std::vector<JsonListParseError>>;

    JsonListWidget(QWidget* pParent = nullptr);
    ~JsonListWidget();

    // Returns entries (empty and commented out items excluded) as '\n' separated string.
    QString entriesAsNewLineSeparatedString() const;

    // Returns entries (empty and commented out items excluded) as QStringList
    QStringList entriesAsStringList() const;

    void setLineWrapping(bool bWrap);

    SyntaxCheckResult checkSyntax() const;
    static QString formatErrorMessage(const SyntaxCheckResult&);

protected:
    void contextMenuEvent(QContextMenuEvent* pEvent) override;

}; // Class JsonListWidget

} } // module namespace
