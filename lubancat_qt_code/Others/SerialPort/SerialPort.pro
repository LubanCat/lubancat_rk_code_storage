QT += widgets serialport
requires(qtConfig(combobox))

TARGET = SerialPort
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

RESOURCES += \
    SerialPort.qrc

target.path = /home/cat/qt/serialport
INSTALLS += target


DESTDIR         = $$PWD/../app_bin
MOC_DIR         = $$PWD/../build/serialport
OBJECTS_DIR     = $$PWD/../build/serialport
