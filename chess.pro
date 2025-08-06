QT += widgets
CONFIG += c++17

SOURCES += main.cpp \
           mainwindow.cpp \
           chessboard.cpp \
           userinformation.cpp \
           promptdialog.cpp

HEADERS += mainwindow.h \
           chessboard.h \
           userinformation.h \
           promptdialog.h

LIBS += -lssl -lcrypto