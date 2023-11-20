QT       += network multimediawidgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtUi
TEMPLATE = lib

DEFINES += QTUI_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS +=  $$PWD/include/qtui_global.h
HEADERS +=  $$PWD/include/QtUi

include($$PWD/src/src.pri)

INCLUDEPATH     += $$PWD/include
DEFINES         += QtUi

DESTDIR         = $$PWD/../libqtui/lib

unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target

unix {
    src_dir = $$PWD/src/*.h
    QtUi = $$PWD/include/QtUi
    dst_dir = $$PWD/../libqtui/include/

    !exists($$dst_dir): system(mkdir -p $$dst_dir)

    system(cp $$src_dir $$dst_dir -arf)
    system(cp $$QtUi $$dst_dir -arf)
}


