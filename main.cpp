#include <QApplication>
#include "mainwindow.h"

/*
tasks.json: We let Qt handle the compilation
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Qt Build (qmake + make)",
            "type": "shell",
            "command": "qmake && make",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        }
    ]
}

c_cpp_properties.json: We include the Qt paths because they are not included in the compiler
{
    "configurations": [
        {
            "name": "Win32",
            "compilerPath": "C:/msys64/mingw64/bin/g++.exe",
            "intelliSenseMode": "windows-gcc-x64",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "includePath": [
                "${workspaceFolder}/**",
                "C:/msys64/mingw64/include",
                "C:/msys64/usr/include",
                "C:/msys64/mingw64/include/QtWidgets",
                "C:/msys64/mingw64/include/QtGui",
                "C:/msys64/mingw64/include/QtCore"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ]
        }
    ],
    "version": 4
}

chess.pro: qmake project file; used by qmake to help include all of the proper files during compilation
QT += widgets
CONFIG += c++17

SOURCES += main.cpp \
           mainwindow.cpp \
           chessboard.cpp
           ... (every .cpp file in the program)

HEADERS += mainwindow.h \
           chessboard.h
           ... (every .h file in the program)


COMPILE CODE:
cd /c/Users/wscal/OneDrive/Desktop/cpp/chess
rm -f Makefile *.o release/chess.exe  // Remove files; delete the auto generated Makefile, all compiled files (end in .o), and the old executable file
qmake  // Reads chess.pro and generate a new Makefile
mingw32-make  // Compile source files (.cpp)
./release/chess.exe  // Execute

*/

// Clarifies number of arguments for the compiler as well as setting them as strings (char pointers point to the first char of the string)
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);  // Creates a Qt application object
    MainWindow w;  // Creates a main window object
    w.show();  // Displays the main window on screen
    return a.exec();  // Starts the Qt event loop; used to process events
}