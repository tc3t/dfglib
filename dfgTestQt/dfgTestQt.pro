#-------------------------------------------------
#
# Project created by QtCreator 2020-03-13T15:15:50
#
#-------------------------------------------------

QT       += core gui sql testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dfgTestQt
TEMPLATE = app

include(../dfg/qt/qmakeTools/dfgQmakeUtil.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS DFG_DEBUG_ASSERT_WITH_THROW=1

INCLUDEPATH += \
    ../externals/ \
    ../

# Below is the original Qt-originated comment related to QT_DISABLE_DEPRECATED_BEFORE -macro
#     You can also make your code fail to compile if you use deprecated APIs.
#     In order to do so, uncomment the following line.
#     You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Sets default C++ standard version.
# If no argument is given, sets default defined in dfgQmakeUtil.pri
$$dfgSetCppVersion()
#$$dfgSetCppVersion("c++20") # Example how to set C++20

CONFIG += console

SOURCES += \
    ../dfg/io/widePathStrToFstreamFriendlyNonWide.cpp \
    ../dfg/math/FormulaParser.cpp \
    ../dfg/os/memoryMappedFile.cpp \
    ../dfg/qt/ConsoleDisplay.cpp \
    ../dfg/qt/CsvItemModel.cpp \
    ../dfg/qt/CsvTableView.cpp \
    ../dfg/qt/CsvTableViewCompleterDelegate.cpp \
    ../dfg/qt/QtApplication.cpp \
    ../dfg/qt/TableEditor.cpp \
    ../dfg/qt/graphTools.cpp \
    ../dfg/qt/qtBasic.cpp \
    ../dfg/qt/CsvItemModelChartDataSource.cpp \
    ../dfg/qt/FileDataSource.cpp \
    ../dfg/qt/CsvFileDataSource.cpp \
    ../dfg/qt/SQLiteFileDataSource.cpp \
    ../dfg/qt/NumberGeneratorDataSource.cpp \
    ../dfg/qt/qxt/gui/qxtspanslider.cpp \
    ../dfg/qt/JsonListWidget.cpp \
    ../dfg/qt/sqlTools.cpp \
    ../dfg/qt/stringConversions.cpp \
    ../dfg/str/fmtlib/fmt.cpp \
    ../dfg/time/DateTime.cpp \
    ../externals/gtest/gtest-all.cc \
    dfgTestCsvItemModel.cpp \
    dfgTestCsvTableView.cpp \
    dfgTestQt.cpp \
    main.cpp

HEADERS += \
    ../dfg/CsvFormatDefinition.hpp \
    ../dfg/qt/ConsoleDisplay.hpp \
    ../dfg/qt/CsvItemModel.hpp \
    ../dfg/qt/CsvTableView.hpp \
    ../dfg/qt/CsvTableViewActions.hpp \
    ../dfg/qt/CsvTableViewCompleterDelegate.hpp \
    ../dfg/qt/InputDialog.hpp \
    ../dfg/qt/PropertyHelper.hpp \
    ../dfg/qt/QtApplication.hpp \
    ../dfg/qt/StringMatchDefinition.hpp \
    ../dfg/qt/TableEditor.hpp \
    ../dfg/qt/TableView.hpp \
    ../dfg/qt/connectHelper.hpp \
    ../dfg/qt/containerUtils.hpp \
    ../dfg/qt/graphTools.hpp \
    ../dfg/qt/qtBasic.hpp \
    ../dfg/qt/qtIncludeHelpers.hpp \
    ../dfg/qt/qxt/gui/qxtspanslider.h \
    ../dfg/qt/qxt/gui/qxtspanslider_p.h \
    ../dfg/qt/tableViewUndoCommands.hpp \
    ../dfg/qt/widgetHelpers.hpp \
    ../dfg/qt/CsvTableViewChartDataSource.hpp \
    ../dfg/qt/CsvItemModelChartDataSource.hpp \
    ../dfg/qt/FileDataSource.hpp \
    ../dfg/qt/CsvFileDataSource.hpp \
    ../dfg/qt/SQLiteFileDataSource.hpp \
    ../dfg/qt/NumberGeneratorDataSource.hpp \
    ../dfg/qt/sqlTools.hpp \
    ../dfg/qt/JsonListWidget.hpp \
    ../dfg/qt/PatternMatcher.hpp \
    ../dfg/qt/stringConversions.hpp \
    ../dfgTest/dfgTest.hpp \
    dfgTestQt_gtestPrinters.hpp

OTHER_FILES += \
    ../dfg/qt/res/chartGuide.html

mingw {
    # To avoid errors in MinGW debug-build, e.g. "x86_64-w64-mingw32/bin/as.exe: debug\CsvTableView.o: too many sections (41408)"
    QMAKE_CXXFLAGS_DEBUG += -Wa,-mbig-obj
}

# PRECOMPILED_HEADER = pch.hpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
