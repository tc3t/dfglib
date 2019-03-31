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

INCLUDEPATH += $$_PRO_FILE_PWD_/../../
INCLUDEPATH += $$_PRO_FILE_PWD_/../../externals/

RESOURCES += \
    dfgqttableeditor.qrc
