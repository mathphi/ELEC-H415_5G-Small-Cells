QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Source/analysisdialog.cpp \
    Source/analysisline.cpp \
    Source/antennas.cpp \
    Source/building.cpp \
    Source/buildingdialog.cpp \
    Source/computationunit.cpp \
    Source/constants.cpp \
    Source/corner.cpp \
    Source/coverageoptimizer.cpp \
    Source/datalegenditem.cpp \
    Source/emitter.cpp \
    Source/emitterdialog.cpp \
    Source/impulsedialog.cpp \
    Source/main.cpp \
    Source/mainwindow.cpp \
    Source/raypath.cpp \
    Source/receiver.cpp \
    Source/receiverdialog.cpp \
    Source/scaleruleritem.cpp \
    Source/simsetupdialog.cpp \
    Source/simulationarea.cpp \
    Source/simulationdata.cpp \
    Source/simulationhandler.cpp \
    Source/simulationitem.cpp \
    Source/simulationscene.cpp \
    Source/walls.cpp

HEADERS += \
    Source/analysisdialog.h \
    Source/analysisline.h \
    Source/antennas.h \
    Source/building.h \
    Source/buildingdialog.h \
    Source/computationunit.h \
    Source/constants.h \
    Source/corner.h \
    Source/coverageoptimizer.h \
    Source/datalegenditem.h \
    Source/emitter.h \
    Source/emitterdialog.h \
    Source/impulsedialog.h \
    Source/mainwindow.h \
    Source/raypath.h \
    Source/receiver.h \
    Source/receiverdialog.h \
    Source/scaleruleritem.h \
    Source/simsetupdialog.h \
    Source/simulationarea.h \
    Source/simulationdata.h \
    Source/simulationhandler.h \
    Source/simulationitem.h \
    Source/simulationscene.h \
    Source/walls.h

FORMS += \
    Source/analysisdialog.ui \
    Source/buildingdialog.ui \
    Source/emitterdialog.ui \
    Source/impulsedialog.ui \
    Source/mainwindow.ui \
    Source/receiverdialog.ui \
    Source/simsetupdialog.ui

DISTFILES += \
    README.md

RESOURCES += \
    Resources/resources.qrc

INCLUDEPATH += Source/

RC_ICONS = Resources/antenna.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
