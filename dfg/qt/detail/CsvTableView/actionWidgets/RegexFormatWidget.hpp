#pragma once

#include "../../../../dfgDefs.hpp"
#include "../../../qtIncludeHelpers.hpp"

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QDialog>
DFG_END_INCLUDE_QT_HEADERS

DFG_ROOT_NS_BEGIN { DFG_SUB_NS(qt) { namespace CsvTableViewActionWidgets {

class RegexFormatWidget : public QDialog
{
public:
    using BaseClass = QDialog;
    RegexFormatWidget(QWidget* pParent);
    ~RegexFormatWidget();

    CsvTableViewActionRegexFormatParams getParams() const;

    QString getRegex() const;
    QString getFormatString() const;

    void setPreviewInput(const QStringList& inputPreview);

private:
    void updatePreviewOutput();
}; // class RegexFormat

}}} // namespace dfg::qt::CsvTableViewActionWidgets
