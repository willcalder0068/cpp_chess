#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QWidget>
#include <QString>
#include <QMap>
#include <QHash>
#include <QMainWindow>
#include <QPair>
#include "mainwindow.h"
#include "chess.hpp"
#include "userinformation.h"

// Allows the key to be retrieved by the point in our guiToFen QHash
inline uint qHash(const QPoint &key, uint seed = 0) {
    return qHash(QPair<int, int>(key.x(), key.y()), seed);
}

class ChessBoard : public QWidget {  // The class is defined as a QWidget; this means that it will automatically call to paintEvent upon construction
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation)

    // Pre-declare method headers to avoid compilation error; we also declare all of our variables here to make the program more concise
    public:
        ChessBoard(QWidget *parent = nullptr);
        QSize sizeHint() const override;  // Used implicitly by Qt to manage widget layouts

        void setInfo(UserInformation* i);  // Used to pass the fields from UserInformation to ChessBoard 
        UserInformation* info = nullptr;  // Will be initialized with info from main

        QPoint selectedSquare;
        QPoint pieceSquare;

        std::vector<QPoint> possiblePieceMoves;
        chess::Movelist legalMoves;

        struct guiPiece { int row; int col; };

        QMap<QString, guiPiece> boardGui;
        chess::Board boardHard;

        // Used to map from FEN and uci to QPointer
        const QHash<QPoint, QString> guiToFen = {
            {{0, 0}, "a8"}, {{1, 0}, "b8"}, {{2, 0}, "c8"}, {{3, 0}, "d8"}, {{4, 0}, "e8"}, {{5, 0}, "f8"}, {{6, 0}, "g8"}, {{7, 0}, "h8"},
            {{0, 1}, "a7"}, {{1, 1}, "b7"}, {{2, 1}, "c7"}, {{3, 1}, "d7"}, {{4, 1}, "e7"}, {{5, 1}, "f7"}, {{6, 1}, "g7"}, {{7, 1}, "h7"},
            {{0, 2}, "a6"}, {{1, 2}, "b6"}, {{2, 2}, "c6"}, {{3, 2}, "d6"}, {{4, 2}, "e6"}, {{5, 2}, "f6"}, {{6, 2}, "g6"}, {{7, 2}, "h6"},
            {{0, 3}, "a5"}, {{1, 3}, "b5"}, {{2, 3}, "c5"}, {{3, 3}, "d5"}, {{4, 3}, "e5"}, {{5, 3}, "f5"}, {{6, 3}, "g5"}, {{7, 3}, "h5"},
            {{0, 4}, "a4"}, {{1, 4}, "b4"}, {{2, 4}, "c4"}, {{3, 4}, "d4"}, {{4, 4}, "e4"}, {{5, 4}, "f4"}, {{6, 4}, "g4"}, {{7, 4}, "h4"},
            {{0, 5}, "a3"}, {{1, 5}, "b3"}, {{2, 5}, "c3"}, {{3, 5}, "d3"}, {{4, 5}, "e3"}, {{5, 5}, "f3"}, {{6, 5}, "g3"}, {{7, 5}, "h3"},
            {{0, 6}, "a2"}, {{1, 6}, "b2"}, {{2, 6}, "c2"}, {{3, 6}, "d2"}, {{4, 6}, "e2"}, {{5, 6}, "f2"}, {{6, 6}, "g2"}, {{7, 6}, "h2"},
            {{0, 7}, "a1"}, {{1, 7}, "b1"}, {{2, 7}, "c1"}, {{3, 7}, "d1"}, {{4, 7}, "e1"}, {{5, 7}, "f1"}, {{6, 7}, "g1"}, {{7, 7}, "h1"},
        };

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;  // Called automatically when the user clicks

    // We put stuff that is only used within the class in private, stuff that is inherited in protected, and stuff that needs to be used elsewhere in public
    private:
        int squareSize;

        // Used to track threefold repitition
        bool threeBool = false;
        std::vector<std::string> threeMoveStale;
        std::vector<std::string> threeMoveCastle;

        chess::Square squareFromQt(QPoint q);
        QPoint qtFromMove(chess::Move mv);
        QPoint qtFromSquare(chess::Square sq);
        QString pieceTypeToQString(chess::PieceType pt);

        void mousePressReview(int row, int col, QPoint selectedSquare);
        void mousePressGame(int row, int col, QPoint selectedSquare);

        void drawBoard(QPainter &painter);
        void drawPieces(QPainter &painter);

        void drawPawn(QPainter &painter, QString color, int row, int col);
        void drawRook(QPainter &painter, QString color, int row, int col);
        void drawBishop(QPainter &painter, QString color, int row, int col);
        void drawKnight(QPainter &painter, QString color, int row, int col);
        void drawQueen(QPainter &painter, QString color, int row, int col);
        void drawKing(QPainter &painter, QString color, int row, int col);

        void gameOverCM();
        void gameOverStale();
        void gameOverIN();
        void gameOverThree();
        void gameOverFifty();
        void gameOver();
};

#endif