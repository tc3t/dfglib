#pragma once

#include <dfg/qt/qtIncludeHelpers.hpp>

DFG_BEGIN_INCLUDE_QT_HEADERS
    #include <QObject>
DFG_END_INCLUDE_QT_HEADERS

class QUrl;

// Implements handling of URL that has scheme dfgdiff;
// in practice means that when clicking link in about box whose url
// starts with dfgdiff, handling will happen in urlHandler() function.
// Handler is activated in setUrlHandler()
class DiffUrlHandler : public QObject
{
    Q_OBJECT

public slots:
    void urlHandler(const QUrl& url);
}; // class DiffUrlHandler
