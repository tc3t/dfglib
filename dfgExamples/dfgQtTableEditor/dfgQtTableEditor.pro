#-------------------------------------------------
#
# Project created by QtCreator 2018-07-13T23:07:39
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dfgQtTableEditor
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

version_time_epoch_start=1556658000 # 2019-05-01 00:00:00
version_time_step=86400 # For now using days (60*60*24) to avoid overflow in VERSION-option (see below)

win32 {
    # With MSVC, old Qt-versions (before 5.5) do some weird stuff with VERSION (https://bugreports.qt.io/browse/QTBUG-44823):
    # From VERSION a.b.c.d, it passes a.bcd to /VERSION link option.
    # As /VERSION expects values from range [0, 65535], version such as 1.6.6.123 causes link error as 66123 > 65535.
    # https://docs.microsoft.com/en-us/cpp/build/reference/version-version-information?view=vs-2015

    # Using PowerShell to determine the time, syntax with the help of https://stackoverflow.com/a/41878110
    version_time = $$system(powershell [math]::floor(((((Get-Date -Date ((Get-Date).ToUniversalTime()) -UFormat %s) - $$version_time_epoch_start))) / $$version_time_step))


} else {
    version_time = $$system(current_time=$(date +%s); echo $((($current_time-$$version_time_epoch_start)/$$version_time_step)))
}

# Application version. Meaning of fields:
# First (left-hand): unspecified
# Second: unspecified
# Third: unspecified
# Last: Time since version_time_epoch_start. Currently whole days since, but may change in the future.
# On Windows VERSION triggers auto-generation of rc-file, https://doc.qt.io/qt-5/qmake-variable-reference.html#version
VERSION = 0.9.9.$$version_time
DEFINES += DFG_QT_TABLE_EDITOR_VERSION_STRING=\\\"$${VERSION}\\\"
message("Version time epoch start is " $$version_time_epoch_start)
message("Version is " $$VERSION)

CONFIG += c++11

win32 {
    # Adjustments on Windows:

    #   -Warning level from W3 -> W4
    QMAKE_CXXFLAGS_WARN_ON -= -W3
    QMAKE_CXXFLAGS_WARN_ON += -W4

    #   -Enable pdb-creation (/Zi /DEBUG)
    #   -Set optimization settings to defaults in Visual Studio Release builds (/OPT:REF /OPT:ICF)
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF /OPT:ICF
}


SOURCES += \
        main.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvItemModel.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableView.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/QtApplication.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/TableEditor.cpp \
        $$_PRO_FILE_PWD_/../../dfg/os/memoryMappedFile.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewCompleterDelegate.cpp

HEADERS += $$_PRO_FILE_PWD_/../../dfg/qt/CsvItemModel.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableView.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewActions.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/QtApplication.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/TableEditor.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/PropertyHelper.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewCompleterDelegate.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/StringMatchDefinition.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/tableViewUndoCommands.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/widgetHelpers.hpp

win32 {
    SOURCES += \
            $$_PRO_FILE_PWD_/../../dfg/debug/structuredExceptionHandling.cpp

# For unknown reason this wouldn't get compiled even after running qmake and rebuilding (Qt Creator 4.9.0). Renaming the file to format.cpp
# would make it work; possibly a qmake bug given:
# https://lists.qt-project.org/pipermail/qt-creator/2017-July/006661.html ("qmake will not build c++ file named something like format.cc and ostream.cc")
# https://bugreports.qt.io/browse/QTBUG-61842
# As a workaround the file has been included in main.cpp
#            $$_PRO_FILE_PWD_/../../dfg/str/fmtlib/format.cc

    HEADERS += $$_PRO_FILE_PWD_/../../dfg/debug/structuredExceptionHandling.h
}

!win32 {
    SOURCES += \
            $$_PRO_FILE_PWD_/../../dfg/io/widePathStrToFstreamFriendlyNonWide.cpp
}

INCLUDEPATH += $$_PRO_FILE_PWD_/../../
INCLUDEPATH += $$_PRO_FILE_PWD_/../../externals/

RESOURCES += \
    dfgqttableeditor.qrc
