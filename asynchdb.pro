TEMPLATE    = app
TARGET      = asynchdb
CONFIG += qt thread
QT = core gui sql
FORMS = win.ui
HEADERS += querythread.h win_impl.h db.h sqlrecmodel.h
SOURCES += main.cpp querythread.cpp win_impl.cpp sqlrecmodel.cpp


