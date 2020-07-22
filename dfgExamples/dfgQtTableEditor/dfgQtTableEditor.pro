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

# Main switches for QtCharts and QCustomPlot. Value can be 1 even if not available; there's a separate availability check later.
# Note: building with Qt charts is not maintained, might not work.
DFG_ALLOW_QT_CHARTS = 0
# Using QCustomPlot requires at least Qt 5.6 (this is a dfglib requirement, QCustomPlot (2.0.1) itself can be used in earlier Qt).
DFG_ALLOW_QCUSTOMPLOT = 0


# -------------------------------------------------------
# Setting modules

QT       = core gui

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

equals(DFG_ALLOW_QT_CHARTS, "1") {
    qtHaveModule(charts) {
        message("Qt Charts module found")
        QT_CHARTS_AVAILABLE=1

    } else {
        message("Qt Charts module NOT found")
        QT_CHARTS_AVAILABLE = 0
    }
}

# Checking whether to use QtCharts
equals(DFGQTE_USING_QCUSTOMPLOT, "0") {
    equals(QT_CHARTS_AVAILABLE, "1") {
        equals(DFG_ALLOW_QT_CHARTS, "1") {
            message("Using Qt Charts. WARNING: using Qt Charts causes GPL infection")
            QT       += charts
            DEFINES += DFG_ALLOW_QT_CHARTS=1 DFGQTE_GPL_INFECTED=1
        } else {
            message("Qt Charts was found, but not used due to config flag")
        }
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

version_time_epoch_start=1595419200 # 2020-07-22 12:00:00Z
version_time_step=86400 # For now using days (60*60*24) to avoid overflow in VERSION-option (see below)

win32 {
    # With MSVC, old Qt-versions (before 5.5) do some weird stuff with VERSION (https://bugreports.qt.io/browse/QTBUG-44823):
    # From VERSION a.b.c.d, it passes a.bcd to /VERSION link option.
    # As /VERSION expects values from range [0, 65535], version such as 1.6.6.123 causes link error as 66123 > 65535.
    # https://docs.microsoft.com/en-us/cpp/build/reference/version-version-information?view=vs-2015

    # Using PowerShell to determine the time, syntax with the help of https://stackoverflow.com/a/41878110
    version_time = $$system(powershell [math]::floor(((((Get-Date -Date ((Get-Date).ToUniversalTime()) -UFormat %s) - $$version_time_epoch_start))) / $$version_time_step))
    version_time_epoch_start_readable = $$system(powershell [System.DateTimeOffset]::FromUnixTimeSeconds($$version_time_epoch_start).UtcDateTime.ToString(\\\"s\\\"))


} else {
    version_time = $$system(current_time=$(date +%s); echo $((($current_time-$$version_time_epoch_start)/$$version_time_step)))
    version_time_epoch_start_readable = $$system(date -u --date='@$$version_time_epoch_start' +%Y-%m-%dT%H:%m:%S)
}

win32 {
    #git_executable_path = "C:\Program Files\Git\bin\git.exe"
} else {
    git_executable_path = ""
}

isEmpty(git_executable_path) {
} else {
    message("git executable path is " $$git_executable_path)
    exists($$git_executable_path) {
        git_branch = $$system($$system_quote($$git_executable_path) rev-parse --abbrev-ref HEAD)
        git_commit = $$system($$system_quote($$git_executable_path) rev-parse HEAD)
        git_diff_stat = $$system($$system_quote($$git_executable_path) diff --stat)
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
VERSION = 1.5.0.$$version_time # Note: when changing version, remember to update version_time_epoch_start
DEFINES += DFG_QT_TABLE_EDITOR_VERSION_STRING=\\\"$${VERSION}\\\"
message("Version time epoch start is " $$version_time_epoch_start_readable " (" $$version_time_epoch_start ")")
message("Version is " $$VERSION)

CONFIG += c++14

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
        $$_PRO_FILE_PWD_/../../dfg/qt/JsonListWidget.cpp \
        $$_PRO_FILE_PWD_/../../dfg/qt/graphTools.cpp

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
        $$_PRO_FILE_PWD_/../../dfg/qt/JsonListWidget.hpp \
        $$_PRO_FILE_PWD_/../../dfg/charts/commonChartTools.hpp \
        $$_PRO_FILE_PWD_/../../dfg/chartsAll.hpp

equals(DFGQTE_USING_QCUSTOMPLOT, "1") {
        HEADERS += $$_PRO_FILE_PWD_/../../dfg/qt/qcustomplot/qcustomplot.h
}

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

!msvc {
    SOURCES += \
            $$_PRO_FILE_PWD_/../../dfg/io/widePathStrToFstreamFriendlyNonWide.cpp
}

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
