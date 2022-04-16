#-------------------------------------------------
#
# Project created by QtCreator 2018-07-13T23:07:39
#
#-------------------------------------------------

message("----------------------------------------------------------------------------------------------------")
message("Starting handling of dfgQtTableEditor.pro")
message("QT_VERSION is " $$QT_VERSION)
message("QMAKESPEC is " $$QMAKESPEC)

# Items below worked, but commented out to reduce verbosity.
#message("QMAKE_HOST.arch is " $$QMAKE_HOST.arch)
#message("QMAKE_HOST.os is " $$QMAKE_HOST.os)
#message("QMAKE_HOST.cpu_count is " $$QMAKE_HOST.cpu_count)
#message("QMAKE_HOST.name is " $$QMAKE_HOST.name)
#message("QMAKE_HOST.version is " $$QMAKE_HOST.version)
#message("QMAKE_HOST.version_string is " $$QMAKE_HOST.version_string)

# Main switch for QCustomPlot. Value can be 1 even if not available; there's a separate availability check later.
# Using QCustomPlot requires at least Qt 5.6 (this is a dfglib requirement, QCustomPlot (2.0.1) itself can be used in earlier Qt).
DFG_ALLOW_QCUSTOMPLOT = 0

DFGQTE_USING_PCH = 1

# -------------------------------------------------------
# Setting modules

QT       = core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Checking whether to use QCustomPlot
exists( $$_PRO_FILE_PWD_/../../dfg/qt/qcustomplot/qcustomplot.cpp ) {
    QCUSTOMPLOT_AVAILABLE = 1
} else {
    QCUSTOMPLOT_AVAILABLE = 0
}

DFGQTE_USING_QCUSTOMPLOT = 0
equals(DFG_ALLOW_QCUSTOMPLOT, "1") {
    equals(QCUSTOMPLOT_AVAILABLE, "1") {
        message("Using QCustomPlot. WARNING: using QCustomPlot causes GPL infection")
        DEFINES += DFG_ALLOW_QCUSTOMPLOT=1 DFGQTE_GPL_INFECTED=1
        DFGQTE_USING_QCUSTOMPLOT = 1
        QT += printsupport
    } else {
        message("QCustomPlot NOT found.")
    }
}

# -------------------------------------------------------

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

# Commands: chosen time to unix time: date --date='yyyy-mm-ddT08:00:00Z' +%s
#           Unix time to ISO date: date -u --date='@<unix time here>' +%Y-%m-%dT%H:%M:%S
version_time_epoch_start=1642579200 # 2022-01-19T08:00:00Z
version_time_step=86400 # For now using days (60*60*24) to avoid overflow in VERSION-option (see below)

win32 {
    # With MSVC, old Qt-versions (before 5.5) do some weird stuff with VERSION (https://bugreports.qt.io/browse/QTBUG-44823):
    # From VERSION a.b.c.d, it passes a.bcd to /VERSION link option.
    # As /VERSION expects values from range [0, 65535], version such as 1.6.6.123 causes link error as 66123 > 65535.
    # https://docs.microsoft.com/en-us/cpp/build/reference/version-version-information?view=vs-2015

    # Using PowerShell to determine the time, syntax with the help of https://stackoverflow.com/a/41878110
    version_time = $$system(powershell [math]::floor(((((Get-Date -Date ((Get-Date).ToUniversalTime()) -UFormat %s) - $$version_time_epoch_start))) / $$version_time_step))
    version_time_epoch_start_readable = $$system(powershell [System.DateTimeOffset]::FromUnixTimeSeconds($$version_time_epoch_start).UtcDateTime.ToString(\\\"s\\\"))
    qmake_run_time = $$system(powershell Get-Date -Date ((Get-Date).ToUniversalTime()) -UFormat %s)
    qmake_run_time_readable = $$system(powershell [System.DateTimeOffset]::FromUnixTimeSeconds($$qmake_run_time).UtcDateTime.ToString(\\\"s\\\"))

} else {
    version_time = $$system(current_time=$(date +%s); echo $((($current_time-$$version_time_epoch_start)/$$version_time_step)))
    version_time_epoch_start_readable = $$system(date -u --date='@$$version_time_epoch_start' +%Y-%m-%dT%H:%m:%S)
    qmake_run_time = $$system(date +%s)
    qmake_run_time_readable = $$system(date -u --date='@$$qmake_run_time' +%Y-%m-%dT%H:%m:%S)
}

message("qmake run at " $$qmake_run_time_readable)

# Note: Setting git_executable_path causes various working tree information such as diff to
#       get embedded into the binary.
win32 {
    #git_executable_path = "C:\Program Files\Git\bin\git.exe"
} else {
    git_executable_path = ""
}

diff_path = "temp_working_tree.diff"
# Making sure that diff file always exists as .qrc refers to it.
write_file($$diff_path) # Writes empty file

isEmpty(git_executable_path) {
} else {
    message("git executable path is " $$git_executable_path)
    exists($$git_executable_path) {
        git_branch = $$system($$system_quote($$git_executable_path) rev-parse --abbrev-ref HEAD)
        git_commit = $$system($$system_quote($$git_executable_path) rev-parse HEAD)
        git_diff_stat = $$system($$system_quote($$git_executable_path) diff --stat)
        message("Printing diff to " $$diff_path)
        diff_file_header = "Working tree diff at $$qmake_run_time_readable"
        write_file($$diff_path, diff_file_header)
        wt_diff = $$system($$system_quote($$git_executable_path) diff, "blob")
        write_file($$diff_path, wt_diff, append)
    } else {
        message("No git-executable found, not defining git info macros")
    }
}

isEmpty(git_branch) {
} else {
    message("git branch is " $$git_branch)
    DEFINES += DFGQTE_GIT_BRANCH=\\\"$$git_branch\\\"
}

isEmpty(git_commit) {
} else {
    message("git commit is " $$git_commit)
    isEmpty(git_diff_stat) {
        DEFINES += DFGQTE_GIT_WORKING_TREE_STATUS=\\\"unmodified\\\"
    } else {
        DEFINES += DFGQTE_GIT_WORKING_TREE_STATUS=\\\"modified\\\"
    }
    DEFINES += DFGQTE_GIT_COMMIT=\\\"$$git_commit\\\"
}


# Application version. Meaning of fields:
# First (left-hand): unspecified
# Second: unspecified
# Third: unspecified
# Last: Time since version_time_epoch_start. Currently whole days since, but may change in the future.
# On Windows VERSION triggers auto-generation of rc-file, https://doc.qt.io/qt-5/qmake-variable-reference.html#version
VERSION = 2.2.1.$$version_time # Note: when changing version, remember to update version_time_epoch_start
DEFINES += DFG_QT_TABLE_EDITOR_VERSION_STRING=\\\"$${VERSION}\\\"
message("Version time epoch start is " $$version_time_epoch_start_readable " (" $$version_time_epoch_start ")")
message("Version is " $$VERSION)

CONFIG += c++17

msvc {
    # Adjustments on MSVC:

    message("Making msvc-specific adjustments")

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
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewCompleterDelegate.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewChartDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvItemModelChartDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/FileDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvFileDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/SQLiteFileDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/NumberGeneratorDataSource.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/JsonListWidget.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/sqlTools.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/graphTools.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/stringConversions.cpp \
        $$_PRO_FILE_PWD_/../../dfg/math/FormulaParser.cpp \
        $$_PRO_FILE_PWD_/../../dfg/str/fmtlib/fmt.cpp \
        $$_PRO_FILE_PWD_/../../dfg/time/DateTime.cpp

equals(DFGQTE_USING_QCUSTOMPLOT, "1") {
        SOURCES += $$_PRO_FILE_PWD_/../../dfg/qt/qcustomplot/qcustomplot.cpp
}

HEADERS += $$_PRO_FILE_PWD_/../../dfg/qt/CsvItemModel.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableView.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewActions.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/QtApplication.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/TableEditor.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/PropertyHelper.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewCompleterDelegate.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/StringMatchDefinition.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/tableViewUndoCommands.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/widgetHelpers.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/graphTools.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/containerUtils.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/TableView.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/ConsoleDisplay.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvTableViewChartDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvItemModelChartDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/FileDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/CsvFileDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/SQLiteFileDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/NumberGeneratorDataSource.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/JsonListWidget.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/sqlTools.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/PatternMatcher.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/InputDialog.hpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/stringConversions.hpp \
        $$_PRO_FILE_PWD_/../../dfg/math/FormulaParser.hpp \
        $$_PRO_FILE_PWD_/../../dfg/CsvFormatDefinition.hpp \
        $$_PRO_FILE_PWD_/../../dfg/charts/commonChartTools.hpp \
        $$_PRO_FILE_PWD_/../../dfg/charts/operations.hpp \
        $$_PRO_FILE_PWD_/../../dfg/chartsAll.hpp \
        main.h

equals(DFGQTE_USING_QCUSTOMPLOT, "1") {
        HEADERS += $$_PRO_FILE_PWD_/../../dfg/qt/qcustomplot/qcustomplot.h
}

equals(DFGQTE_USING_PCH, "1") {
    PRECOMPILED_HEADER = pch.hpp
    message("Using precompiled header $$PRECOMPILED_HEADER")
}

win32 {
    SOURCES += \
            $$_PRO_FILE_PWD_/../../dfg/debug/structuredExceptionHandling.cpp

    HEADERS += $$_PRO_FILE_PWD_/../../dfg/debug/structuredExceptionHandling.h
}

!msvc {
    SOURCES += \
            $$_PRO_FILE_PWD_/../../dfg/io/widePathStrToFstreamFriendlyNonWide.cpp
}

OTHER_FILES += \
    $$_PRO_FILE_PWD_/../../dfg/qt/res/chartGuide.html

INCLUDEPATH += $$_PRO_FILE_PWD_/../../
INCLUDEPATH += $$_PRO_FILE_PWD_/../../externals/

win32 {
    # This sets file icon for the .exe file.
    # For details, see "Setting the Application Icon" from Qt documentation.
    RC_ICONS = executable_file_icon.ico
}

RESOURCES += \
    dfgqttableeditor.qrc

message("Using modules: " $$QT)
message("TARGET is " $$TARGET)
message("CONFIG is " $$CONFIG)
message("DEFINES is " $$DEFINES)
message("QMAKE_CXXFLAGS is " $$QMAKE_CXXFLAGS)
message("QMAKE_CXXFLAGS_DEBUG is " $$QMAKE_CXXFLAGS_DEBUG)
message("QMAKE_CXXFLAGS_RELEASE is " $$QMAKE_CXXFLAGS_RELEASE)
message("QMAKE_LFLAGS is " $$QMAKE_LFLAGS)
message("QMAKE_LFLAGS_DEBUG is " $$QMAKE_LFLAGS_DEBUG)
message("QMAKE_LFLAGS_RELEASE is " $$QMAKE_LFLAGS_RELEASE)
message("QMAKE_CXXFLAGS_WARN_ON is " $$QMAKE_CXXFLAGS_WARN_ON)
message("QMAKE_CXXFLAGS_WARN_OFF is " $$QMAKE_CXXFLAGS_WARN_OFF)
message("^^^^^Done with dfgQtTableEditor.pro handling^^^^^^")
