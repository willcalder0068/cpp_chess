#ifndef CHESSBOARD_H
#define CHESSBOARD_H

#include <QWidget>
#include <QString>
#include <QMap>

class ChessBoard : public QWidget {  // The class is defined as a QWidget; this means that it will automatically call to paintEvent upon construction
    Q_OBJECT  // Declare the class as a Qt meta-object, meaning it can be manipulated at runtime (after compilation)

    // Pre-declare method headers to avoid compilation error; we also declare all of our variables here to make the program more concise
    public:
        ChessBoard(QWidget *parent = nullptr);
        QSize sizeHint() const override;

        struct Piece { int row; int col; };
        QMap<QString, Piece> boardState = {
            {"w a pawn", {6, 0} }, {"w b pawn", {6, 1} }, {"w c pawn", {6, 2} }, {"w d pawn", {6, 3} }, 
            {"w e pawn", {6, 4} }, {"w f pawn", {6, 5} }, {"w g pawn", {6, 6} }, {"w h pawn", {6, 7} }, 
            {"b a pawn", {1, 0} }, {"b b pawn", {1, 1} }, {"b c pawn", {1, 2} }, {"b d pawn", {1, 3} }, 
            {"b e pawn", {1, 4} }, {"b f pawn", {1, 5} }, {"b g pawn", {1, 6} }, {"b h pawn", {1, 7} },

            {"w a rook", {7, 0} }, {"w h rook", {7, 7} }, {"b a rook", {0, 0} }, {"b h rook", {0, 7} },

            {"w b knight", {7, 1} }, {"w g knight", {7, 6} }, {"b b knight", {0, 1} }, {"b g knight", {0, 6} }, 

            {"w c bishop", {7, 2} }, {"w f bishop", {7, 5} }, {"b c bishop", {0, 2} }, {"b f bishop", {0, 5} },

            {"w queen", {7, 3} }, {"b queen", {0, 3} },       {"w king", {7, 4} }, {"b king", {0, 4} }
        };

    protected:
        void paintEvent(QPaintEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;

    private:
        int squareSize;
        QPoint selectedSquare;

        void drawBoard(QPainter &painter);
        void drawPieces(QPainter &painter);

        void drawPawn(QPainter &painter, QString color, int row, int col);
        void drawRook(QPainter &painter, QString color, int row, int col);
        void drawBishop(QPainter &painter, QString color, int row, int col);
        void drawKnight(QPainter &painter, QString color, int row, int col);
        void drawQueen(QPainter &painter, QString color, int row, int col);
        void drawKing(QPainter &painter, QString color, int row, int col);
};

#endif