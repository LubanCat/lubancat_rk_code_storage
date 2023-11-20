TEMPLATE = app
INCLUDEPATH += .

HEADERS     = bookdelegate.h bookwindow.h initdb.h
RESOURCES   = books.qrc
SOURCES     = bookdelegate.cpp main.cpp bookwindow.cpp
FORMS       = bookwindow.ui

QT += sql widgets widgets
requires(qtConfig(tableview))

target.path = $$[QT_INSTALL_EXAMPLES]/sql/drilldown

INSTALLS += target

TARGET = Book

DESTDIR         = $$PWD/../../app_bin/sql
MOC_DIR         = $$PWD/../../build/sql/book
OBJECTS_DIR     = $$PWD/../../build/sql/book
