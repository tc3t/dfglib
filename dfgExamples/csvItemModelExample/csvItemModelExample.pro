TEMPLATE = app
TARGET = csvItemModelExample
QT += core gui widgets sql

INCLUDEPATH += .
INCLUDEPATH += ./../../
INCLUDEPATH += ./../../externals/

HEADERS += ../../dfg/qt/containerUtils.hpp \
    ../../dfg/qt/CsvItemModel.hpp \
    ../../dfg/qt/CsvTableView.hpp \
    ../../dfg/qt/CsvTableViewCompleterDelegate.hpp \
    ../../dfg/qt/JsonListWidget.hpp \
    ../../dfg/qt/TableEditor.hpp \
    ../../dfg/math/FormulaParser.hpp \
    ../../dfg/qt/sqlTools.hpp

SOURCES += ../../dfg/os/memoryMappedFile.cpp \
    ../../dfg/io/widePathStrToFstreamFriendlyNonWide.cpp \
    ../../dfg/qt/CsvItemModel.cpp \
    ../../dfg/qt/CsvTableView.cpp \
    ../../dfg/qt/CsvTableViewCompleterDelegate.cpp \
    ../../dfg/qt/JsonListWidget.cpp \
    ../../dfg/qt/QtApplication.cpp \
    ../../dfg/qt/TableEditor.cpp \
    ../../dfg/qt/sqlTools.cpp \
    ../../dfg/math/FormulaParser.cpp \
    ../../dfg/qt/stringConversions.cpp \
    ../../dfg/time/DateTime.cpp \
    ./main.cpp

RESOURCES += CsvItemModelExample.qrc
