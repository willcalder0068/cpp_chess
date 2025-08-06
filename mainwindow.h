#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chessboard.h"

// Extend the QMainWindow class that we inherit from Qt
class MainWindow : public QMainWindow {
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation); creates a moc_ file

    // Pre-declare method headers to avoid compilation errors (what we do in all header files)
    public:
        ChessBoard* board = nullptr;  // Declare the reference to the board in public so it can be accessed anywhere in the program

        MainWindow(QWidget *parent = nullptr);
        void moveEvent(QMoveEvent *event);
        ~MainWindow();

    // Signals connect to slot actions; the signal is triggered, and the slot acts
    signals:
        void geometryChanged();
    };

#endif