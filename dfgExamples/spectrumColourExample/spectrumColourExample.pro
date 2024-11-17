TEMPLATE = app
TARGET = spectrumColourExample
QT += core widgets
INCLUDEPATH += .
INCLUDEPATH += ./../../
INCLUDEPATH += ./../../externals/

include(../../dfg/qt/qmakeTools/dfgQmakeUtil.pri)
$$dfgSetCppVersion()
$$dfgSetBuildTypeMacros()

HEADERS += ./spectrumColourExample.h \
    ./stdafx.h

SOURCES += ./main.cpp \
    ../../dfg/os/memoryMappedFile.cpp \
    ./spectrumColourExample.cpp \
    ./stdafx.cpp

FORMS += ./spectrumColourExample.ui

RESOURCES += spectrumColourExample.qrc
