#include <QApplication>
#include <QMessageBox>
#include "mainwindow.h"
#include "userinformation.h"
#include "promptdialog.h"
#include "chessboard.h"

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
                "C:/msys64/mingw64/include/QtCore",
                "C:/Users/wscal/OneDrive/Desktop/cpp/chess/extern"
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

INCLUDEPATH += $$PWD/extern

SOURCES += main.cpp \
           mainwindow.cpp \
           chessboard.cpp \
           userinformation.cpp \
           promptdialog.cpp

HEADERS += mainwindow.h \
           chessboard.h \
           userinformation.h \
           promptdialog.h \
           extern/chess.hpp

LIBS += -lssl -lcrypto


COMPILE CODE:
cd /c/Users/wscal/OneDrive/Desktop/cpp/chess           // Navigate to the correct directory
rm -f release/chess.exe                                // Delete the old executable to compile the new one (incase there are any changes in the code)
qmake                                                  // Reads chess.pro and generates new Makefiles
mingw32-make                                           // Compile source files (.cpp -> .o)
./release/chess.exe                                    // Execute

    **If we have drastically altered the .pro file or include paths: rm -f Makefile* release/*.o release/chess.exe release/moc_* .qmake.stash
        ^^Remove all files used in previous compile to ensure a fresh rebuild (* means all files starting or ending with, depending on its location)

*/

// Clarifies number of arguments for the compiler as well as setting them as strings (char pointers point to the first char of the string)
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);  // Create the application object (which manages the event loop)

    MainWindow w;  // Instantiate the MainWindow object with an implicit call to its construtor ( MainWindow w = MainWindow(); )
    w.show();  // Display the MainWindow

    int iUserChoice = -1;

    // Loop until valid input
    while (true) {
        PromptDialog prompt("Press 1 to register or 2 to login: ", &w);
        prompt.followParent();  // Position under main windows
        QObject::connect(&w, &MainWindow::geometryChanged, &prompt, &PromptDialog::followParent);  // Connect the geometryChanged signal to the followParent slot
        prompt.exec();  // Display the dialog; program will move to the next line when the OK button is clicked

        bool isInt = true;
        std::string sUserChoice = prompt.getInputText().toStdString();

        // Try to convert to int; if the user typed a non int, fall through to the QMessageBox warning
        try {
            iUserChoice = std::stoi(sUserChoice);
        } catch (...) { isInt = false; }

        if (isInt) {
            if (iUserChoice == 1 || iUserChoice == 2) { break; }  // Break and instantiate UserInformation outside of the loop
            else if (iUserChoice == 0) { return 0; }  // Terminate program
        }

        QMessageBox::warning(&w, "Invalid Input", "Please enter 1 to register or 2 to login.");  // Show invalid input message, reloop
    }

    UserInformation info(iUserChoice, &w);  // Instatiate UserInformation; same as: UserInformation info = UserInformation(iUserchoice, &w)

    w.board->setInfo(&info);  // Make the data in UserInformation accessible for our ChessBoard instance

    return a.exec();  // Start the Qt event loop, allowing the user to interact with the board (starts after we have finished with UserInformation)
}