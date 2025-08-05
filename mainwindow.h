#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "chessboard.h"

// Extend the QMainWindow class that we inherit from Qt
class MainWindow : public QMainWindow {
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation)

    // Pre-declare method headers to avoid compilation errors
    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();
    };

#endif