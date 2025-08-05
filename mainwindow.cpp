#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)  // Defining a member of the class MainWindow (MainWindow:); this member is the constructor (:MainWindow)
    : QMainWindow(parent) {
        setCentralWidget(new ChessBoard(this));  // Create the ChessBoard widget
        setWindowTitle("Chess");
        resize(480, 600);  // Board is 480 x 480; other information displayed below the board
}

MainWindow::~MainWindow() {}