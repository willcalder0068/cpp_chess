#include <QMoveEvent>
#include "mainwindow.h"
#include "chessboard.h"

// Defining a member of the class MainWindow (MainWindow:); this member is the constructor (:MainWindow)
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
        board = new ChessBoard(parent);  // Instantiate the board
        setCentralWidget(board);  // Create the ChessBoard widget
        setWindowTitle("Chess | Press '0' to Exit");
        resize(480, 600);  // Board is 480 x 480; other information displayed below the board
}

// Whenever the MainWindow is moved, emit the geometryChanged signal
void MainWindow::moveEvent(QMoveEvent *event) {
    QMainWindow::moveEvent(event);
    emit geometryChanged();
}

MainWindow::~MainWindow() {}