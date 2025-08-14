QT += widgets
CONFIG += c++17

INCLUDEPATH += $$PWD/extern

SOURCES += main.cpp \
           mainwindow.cpp \
           chessboard.cpp \
           userinformation.cpp \
           promptdialog.cpp \


HEADERS += mainwindow.h \
           chessboard.h \
           userinformation.h \
           promptdialog.h \
           runstockfish.h \
           extern/chess.hpp

LIBS += -lssl -lcrypto