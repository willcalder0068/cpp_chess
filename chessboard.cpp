#include "chessboard.h"
#include <QPainter>
#include <QMouseEvent>

ChessBoard::ChessBoard(QWidget *parent)
    : QWidget(parent), squareSize(60), selectedSquare(-1, -1) {
        setMinimumSize(8 * squareSize, 10 * squareSize);
}

// Used implicitly by Qt to manage widget layouts
QSize ChessBoard::sizeHint() const {
    return QSize(8 * squareSize, 10 * squareSize);
}

// Automatically called upon construction; also called with a mousePressEvent (user clicks screen)
void ChessBoard::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    drawBoard(painter);
    drawPieces(painter);
}

// Called whenever the player clicks the screen
void ChessBoard::mousePressEvent(QMouseEvent *event) {
    // Coordinates divided by square size gives the clicked position in terms of squares, not pixels
    int row = event->pos().y() / squareSize;
    int col = event->pos().x() / squareSize;
    selectedSquare = QPoint(col, row);
    update();  // calls paintEvent again
}

void ChessBoard::drawBoard(QPainter &painter) {
    QColor light(187, 196, 200);
    QColor dark(96, 125, 139);

    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            QRect rect(col * squareSize, row * squareSize, squareSize, squareSize);
            painter.fillRect(rect, (row + col) % 2 ? dark : light);  // Drawing the squares of the board

            // If the square is selected, highlight it; if the click is outside of the board, nothing happens
            if (selectedSquare == QPoint(col, row)) {
                painter.setBrush(QColor(255, 100, 100, 120));
                painter.drawRect(rect);
            }
        }
    }
}

// We pull the pieces location from the map and draw them accordingly
void ChessBoard::drawPieces(QPainter &painter) {
    drawPawn(painter, "w", boardState["w a pawn"].row, boardState["w a pawn"].col);
    drawPawn(painter, "w", boardState["w b pawn"].row, boardState["w b pawn"].col);
    drawPawn(painter, "w", boardState["w c pawn"].row, boardState["w c pawn"].col);
    drawPawn(painter, "w", boardState["w d pawn"].row, boardState["w d pawn"].col);
    drawPawn(painter, "w", boardState["w e pawn"].row, boardState["w e pawn"].col);
    drawPawn(painter, "w", boardState["w f pawn"].row, boardState["w f pawn"].col);
    drawPawn(painter, "w", boardState["w g pawn"].row, boardState["w g pawn"].col);
    drawPawn(painter, "w", boardState["w h pawn"].row, boardState["w h pawn"].col);
    drawPawn(painter, "b", boardState["b a pawn"].row, boardState["b a pawn"].col);
    drawPawn(painter, "b", boardState["b b pawn"].row, boardState["b b pawn"].col);
    drawPawn(painter, "b", boardState["b c pawn"].row, boardState["b c pawn"].col);
    drawPawn(painter, "b", boardState["b d pawn"].row, boardState["b d pawn"].col);
    drawPawn(painter, "b", boardState["b e pawn"].row, boardState["b e pawn"].col);
    drawPawn(painter, "b", boardState["b f pawn"].row, boardState["b f pawn"].col);
    drawPawn(painter, "b", boardState["b g pawn"].row, boardState["b g pawn"].col);
    drawPawn(painter, "b", boardState["b h pawn"].row, boardState["b h pawn"].col);

    drawKnight(painter, "w", boardState["w b knight"].row, boardState["w b knight"].col);
    drawKnight(painter, "w", boardState["w g knight"].row, boardState["w g knight"].col);
    drawKnight(painter, "b", boardState["b b knight"].row, boardState["b b knight"].col);
    drawKnight(painter, "b", boardState["b g knight"].row, boardState["b g knight"].col);

    drawBishop(painter, "w", boardState["w c bishop"].row, boardState["w c bishop"].col);
    drawBishop(painter, "w", boardState["w f bishop"].row, boardState["w f bishop"].col);
    drawBishop(painter, "b", boardState["b c bishop"].row, boardState["b c bishop"].col);
    drawBishop(painter, "b", boardState["b f bishop"].row, boardState["b f bishop"].col);

    drawRook(painter, "w", boardState["w a rook"].row, boardState["w a rook"].col);
    drawRook(painter, "w", boardState["w h rook"].row, boardState["w h rook"].col);
    drawRook(painter, "b", boardState["b a rook"].row, boardState["b a rook"].col);
    drawRook(painter, "b", boardState["b h rook"].row, boardState["b h rook"].col);

    drawQueen(painter, "w", boardState["w queen"].row, boardState["w queen"].col);
    drawQueen(painter, "b", boardState["b queen"].row, boardState["b queen"].col);

    drawKing(painter, "w", boardState["w king"].row, boardState["w king"].col);
    drawKing(painter, "b", boardState["b king"].row, boardState["b king"].col);
}

void ChessBoard::drawPawn(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/pawnwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/pawnblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}

void ChessBoard::drawKnight(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/knightwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/knightblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}

void ChessBoard::drawBishop(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/bishopwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/bishopblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}

void ChessBoard::drawRook(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/rookwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/rookblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}

void ChessBoard::drawQueen(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/queenwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/queenblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}

void ChessBoard::drawKing(QPainter &painter, QString color, int row, int col) {
    int x = col * squareSize;
    int y = row * squareSize;

    QPixmap pixmap;
    if (color == "w")
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/kingwhitedrawing.png");
    else
        pixmap.load("C:/Users/wscal/OneDrive/Desktop/cpp/chess/pieces/kingblackdrawing.png");

    painter.drawPixmap(x, y, squareSize, squareSize, pixmap);
}