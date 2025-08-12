#include "chessboard.h"
#include "promptdialog.h"
#include <QPainter>
#include <QMouseEvent>
#include <QString>
#include <QMessageBox>
#include <QTimer>
#include <algorithm>

ChessBoard::ChessBoard(QWidget *parent)
    : QWidget(parent), selectedSquare(-1, -1), squareSize(60) {
        setMinimumSize(8 * squareSize, 10 * squareSize);

        // FEN: Lists ranks (separated by '/', numbers represent empty), color to move, castling abilities (capitol is white, lowercase is black),
        // En passant square (- if not possible, something like e6 if it is), halfmove counter, fullmove counter
        boardHard = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

        // List holds positions; when we get three consecutive with the same position and castle rights, its a threefold repitition)
        QStringList threeMoves = QString::fromStdString(boardHard.getFen()).split(' ');
        threeMoveStale.push_back(threeMoves[0].toStdString());
        threeMoveCastle.push_back(threeMoves[2].toStdString());

        boardGui = {
            {"w a pawn", {6, 0} },   {"w b pawn", {6, 1} },   {"w c pawn", {6, 2} },   {"w d pawn", {6, 3} }, 
            {"w e pawn", {6, 4} },   {"w f pawn", {6, 5} },   {"w g pawn", {6, 6} },   {"w h pawn", {6, 7} }, 
            {"b a pawn", {1, 0} },   {"b b pawn", {1, 1} },   {"b c pawn", {1, 2} },   {"b d pawn", {1, 3} }, 
            {"b e pawn", {1, 4} },   {"b f pawn", {1, 5} },   {"b g pawn", {1, 6} },   {"b h pawn", {1, 7} },
            {"w a rook", {7, 0} },   {"w h rook", {7, 7} },   {"b a rook", {0, 0} },   {"b h rook", {0, 7} },
            {"w b knight", {7, 1} }, {"w g knight", {7, 6} }, {"b b knight", {0, 1} }, {"b g knight", {0, 6} }, 
            {"w c bishop", {7, 2} }, {"w f bishop", {7, 5} }, {"b c bishop", {0, 2} }, {"b f bishop", {0, 5} },
            {"w queen", {7, 3} },    {"b queen", {0, 3} },    {"w king", {7, 4} },     {"b king", {0, 4} }
        };
}

// Used implicitly by Qt to manage widget layouts
QSize ChessBoard::sizeHint() const {
    return QSize(8 * squareSize, 10 * squareSize);
}

// Pass a copy of UserInformation to the class so we can access its public members
void ChessBoard::setInfo(UserInformation* i) {
    info = i;
}

// Automatically called upon construction; also called within mousePressEvent (user clicks screen)
void ChessBoard::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    drawBoard(painter);
    drawPieces(painter);

    // If user info has been passed to the class (login completed)
    if (info) {
        // Small text saying who we are playing
        if (info->elo >= 500) {
            painter.setPen(Qt::black);
            painter.drawText(380, 500, QString::fromStdString("versus " + std::to_string(info->elo) + " elo"));
        }
        else if (info->elo == 1) {
            painter.setPen(Qt::black);
            painter.drawText(380, 500, QString::fromStdString("versus a friend"));
        }
        // Small text telling the user what color he is
        if(info->isWhite) {
            painter.setPen(Qt::black);
            painter.drawText(30, 500, QString::fromStdString("user is white"));
        }
        else {
            painter.setPen(Qt::black);
            painter.drawText(30, 500, QString::fromStdString("user is black"));
        }
    }
}

// Called whenever the player clicks the screen
void ChessBoard::mousePressEvent(QMouseEvent *event) {
    // Coordinates divided by square size gives the clicked position in terms of squares, not pixels
    int row = event->pos().y() / squareSize;
    int col = event->pos().x() / squareSize;
    selectedSquare = QPoint(col, row);

    if (!info) { mousePressGame(row, col, selectedSquare); }
    else {
        if (info->gameMode == 1) { mousePressReview(row, col, selectedSquare); }
        else { mousePressGame(row, col, selectedSquare); }
    }
}

void ChessBoard::mousePressReview(int row, int col, QPoint selectedSquare) {
    /*
        (1) Enable replay mode - add alternative method to promptForElo (promptForReview) in userinformation, add more stuff here
        (2) Comment all of the changes as well as improving the structure and efficiency
        
        (3) Computer moves
    */
}

void ChessBoard::mousePressGame(int row, int col, QPoint selectedSquare) {
    QStringList fenParts = QString::fromStdString(boardHard.getFen()).split(' ');

    // First move buffer to avoid runtime issues; also, the first time through legalMoves is uninitialized
    if (boardHard.getFen() != "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
        // Check for game over's
        if (legalMoves.empty() && boardHard.inCheck()) {
            info->writeCM(info->username, fenParts[1].toStdString());
            gameOverCM();  // Checkmate
        }
        if (legalMoves.empty() && !boardHard.inCheck()) {
            info->writeStale(info->username);
            gameOverStale();  // Stalemate
        }
        if (boardHard.isInsufficientMaterial()) {
            info->writeIN(info->username);
            gameOverIN();  // Insufficient material
        }
        if (threeBool) {
            threeBool = false;
            info->writeThree(info->username);
            gameOverThree();  // Threefold repition
        }
        // Check if fifty-move draw condition triggered
        if (fenParts[4] == "100") {
            info->writeFifty(info->username);
            gameOverFifty();  // Fifty move rule
        }
    }

    legalMoves.clear();  // New position, clear previous moves
    chess::movegen::legalmoves(legalMoves, boardHard);  // Fill the container with all legal moves

    // If the click was on the board
    if (selectedSquare.x() >= 0 && selectedSquare.x() <=7 && selectedSquare.y() >= 0 && selectedSquare.y() <= 7) {
        chess::Square currSquare = squareFromQt(selectedSquare);  // Translate from QPoint to chess::Square

        // If the selected square is currently listed as possible move, then the user wants to move there
        if (std::find(possiblePieceMoves.begin(), possiblePieceMoves.end(), selectedSquare) != possiblePieceMoves.end()) {
            possiblePieceMoves.clear();  // So the next select doesn't retain the old highlighted squares

            // Finds the key within boardGui of the piece we are moving (eg: "w a rook")
            QString movingKey;
            for (auto it = boardGui.begin(); it != boardGui.end(); ++it) {
                if (it.value().row == pieceSquare.y() && it.value().col == pieceSquare.x()) {
                    movingKey = it.key();
                    break;
                }
            }
            // Make sure that we have a piece (we should, this is just to be safe)
            if (!movingKey.isEmpty()) {
                QPoint removeSquare = selectedSquare;  // Must capture enemy pieces

                QString currFen = QString::fromStdString(boardHard.getFen());

                // If the FEN contains the selected Square (instead of '-') AND a pawn is being moved, then the user has done an en passant
                if (guiToFen.contains(selectedSquare) && currFen.contains(guiToFen[selectedSquare]) && boardHard.at(squareFromQt(pieceSquare)) == chess::PieceType::PAWN) {
                    if (currFen.contains("w"))  // If white is moving... (we haven't yet updated the boardHard)
                        removeSquare = QPoint(col, row + 1);  // Remove the piece in the square below
                    else  // If black is moving
                        removeSquare = QPoint(col, row - 1);  // Remove the piece in the square above
                }

                // If a piece was already here, erase it (it has been captured)
                for (auto it = boardGui.begin(); it != boardGui.end(); ++it) {
                    if (it.value().row == removeSquare.y() && it.value().col == removeSquare.x()) {
                        boardGui.erase(it);
                        break;
                    }
                }

                // If the pawn makes it all the way across, promote it to a queen
                if (boardHard.at(squareFromQt(pieceSquare)) == chess::PieceType::PAWN) { 
                    // White pawn all the way at the top or a black pawn all the way at the bottom
                    if ((selectedSquare.y() == 0 && fenParts[1] == "w") || (selectedSquare.y() == 7 && fenParts[1] == "b")) {
                        QStringList keyParts = movingKey.split(' ');  // e.g. "w a pawn" → ["w", "a", "pawn"]
                        QString oldKey = movingKey;
                        movingKey = keyParts[0] + " " + keyParts[1] + " queen";  // Promote the pawn (for the Gui)
                        boardGui.remove(oldKey);  // Remove the pawn key (so it is not redrawn, it is a queen now)
                    }
                }

                // Check for castle
                if (boardHard.at(squareFromQt(pieceSquare)) == chess::PieceType::KING && boardHard.at(squareFromQt(selectedSquare)) == chess::PieceType::ROOK) {
                    if (selectedSquare.x() == 0) {  // Castle queenside
                        if (currFen.contains("w")) {
                            boardGui["w a rook"] = {7, 3};
                            boardGui["w king"] = {7, 2};
                        }
                        else {
                            boardGui["b a rook"] = {0, 3};
                            boardGui["b king"] = {0, 2};
                        }
                    }
                    else {  // Castle kingside
                        if (currFen.contains("w")) {
                            boardGui["w h rook"] = {7, 5};
                            boardGui["w king"] = {7, 6};
                        }
                        else {
                            boardGui["b h rook"] = {0, 5};
                            boardGui["b king"] = {0, 6};
                        }
                    }
                }
                else  // Non castle move
                    boardGui[movingKey] = { selectedSquare.y(), selectedSquare.x() };   // Update boardGui with the new move

                // Make the move on boardHard and write it to the user file
                for (const chess::Move& move : legalMoves) {
                    if (move.from() == squareFromQt(pieceSquare) && move.to() == squareFromQt(selectedSquare)) {
                        std::string fromIndex = guiToFen[pieceSquare].toStdString();
                        std::string name = pieceTypeToQString(boardHard.at(squareFromQt(pieceSquare)).type()).toStdString();
                        std::string toIndex = guiToFen[selectedSquare].toStdString();

                        // Write our move to the user record
                        if (info && !info->username.empty()) {
                            info->writeMove(info->username, (fromIndex + " " + name + " " + toIndex));
                        }

                        boardHard.makeMove(move);  // Update boardHard with the new move

                        // Check for threefold repitition
                        QStringList threeMoves = QString::fromStdString(boardHard.getFen()).split(' ');
                        if (threeMoves[3] == "-") {  // If the move has en passant rights, it cannot be repeated
                            threeMoveStale.push_back(threeMoves[0].toStdString());  // Add on the relevant part of the FEN
                            threeMoveCastle.push_back(threeMoves[2].toStdString());  // Check for changes in castling rights
                            if (threeMoveStale.size() == 10) {
                                // If we have three repeated positions including unchanged castling rights
                                if (threeMoveStale[0] == threeMoveStale[4] && threeMoveStale[0] == threeMoveStale[8] && 
                                    threeMoveStale[1] == threeMoveStale[5] && threeMoveStale[1] == threeMoveStale[9] &&
                                    threeMoveCastle[0] == threeMoveCastle[8] && threeMoveCastle[1] == threeMoveCastle[9]) {
                                        threeBool = true;  // Trigger a three move tie on the next mousePressEvent
                                }
                                // Shift down our vectors and resize them to 9 so we can push the next move
                                for (int i = 0; i <= 8; i++) {
                                    threeMoveStale[i] = threeMoveStale[i + 1];
                                    threeMoveCastle[i] = threeMoveCastle[i + 1];
                                }
                                threeMoveStale.erase(threeMoveStale.begin() + 9);
                                threeMoveCastle.erase(threeMoveCastle.begin() + 9);
                            }
                        }

                        break;
                    }
                }
            }
        }
        // If the user clicked a non highlited square, they are not moving pieces
        else {
            possiblePieceMoves.clear();  // So the next select doesn't retain the old squares
            pieceSquare = selectedSquare;  // Store this as our selected piece for movement later

            for (const auto& move : legalMoves) {
                if (move.from() == currSquare) {
                    possiblePieceMoves.push_back(qtFromMove(move));  // Add all of the points for selection highlighting
                }
            }
        }

        update();  // calls paintEvent
    }
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

            // If one of the points is in possiblePieceMoves, highlight it with a yellow circle
            if (!(possiblePieceMoves.empty())) {
                for (const QPoint &q : possiblePieceMoves) {
                    if (q == QPoint(col, row)) {
                        painter.setBrush(QColor(255, 255, 0, 180));
                        painter.setPen(Qt::NoPen);
                        QRectF circleRect(
                            col * squareSize + squareSize * 0.25,
                            row * squareSize + squareSize * 0.25,
                            squareSize * 0.5,
                            squareSize * 0.5
                        );
                        painter.drawEllipse(circleRect);
                    }
                }
            }
        }
    }
}

// We pull the pieces location from the map and draw them accordingly
void ChessBoard::drawPieces(QPainter &painter) {
    for (auto it = boardGui.begin(); it != boardGui.end(); ++it) {
        const QString &key = it.key();
        const guiPiece &pos = it.value();

        QStringList parts = key.split(' ');  // e.g. "w a pawn" → ["w", "a", "pawn"]

        QString color = parts[0];
        QString pieceType = parts.last();  // last is more reliable than assuming fixed indexes

        if (pieceType == "pawn")
            drawPawn(painter, color, pos.row, pos.col);
        else if (pieceType == "rook")
            drawRook(painter, color, pos.row, pos.col);
        else if (pieceType == "knight")
            drawKnight(painter, color, pos.row, pos.col);
        else if (pieceType == "bishop")
            drawBishop(painter, color, pos.row, pos.col);
        else if (pieceType == "queen")
            drawQueen(painter, color, pos.row, pos.col);
        else if (pieceType == "king")
            drawKing(painter, color, pos.row, pos.col);
    }
}

void ChessBoard::gameOverCM() {
    QStringList fenPartsCM = QString::fromStdString(boardHard.getFen()).split(' ');

    if (fenPartsCM[1] == "b")
        QMessageBox::information(this, "Checkmate", "White wins!");  // Blacks turn to move, they can't
    else
        QMessageBox::information(this, "Checkmate", "Black wins!");  // Whites turn to move, they can't
    gameOver();
}

void ChessBoard::gameOverStale() {
    QMessageBox::information(this, "Stalemate", "It's a Draw!");
    gameOver();
}

void ChessBoard::gameOverIN() {
    QMessageBox::information(this, "Insufficient Material", "It's a Draw!");
}

void ChessBoard::gameOverThree() {
    QMessageBox::information(this, "Threefold Repitition", "It's a Draw!");
    gameOver();
}

void ChessBoard::gameOverFifty() {
    QMessageBox::information(this, "Fifty Move Rule", "It's a Draw!");
    gameOver();
}

void ChessBoard::gameOver() {
    // Reset old board data
    boardHard = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Clear threefold trackers
    threeMoveStale.clear();
    threeMoveCastle.clear();

    // Re initialize threefold trackers
    QStringList threeMoves = QString::fromStdString(boardHard.getFen()).split(' ');
    threeMoveStale.push_back(threeMoves[0].toStdString());
    threeMoveCastle.push_back(threeMoves[2].toStdString());

    chess::movegen::legalmoves(legalMoves, boardHard);  // Fil the container with all legal moves for the fresh board

    // Reset Gui board data
    boardGui = {
        {"w a pawn", {6, 0} },   {"w b pawn", {6, 1} },   {"w c pawn", {6, 2} },   {"w d pawn", {6, 3} }, 
        {"w e pawn", {6, 4} },   {"w f pawn", {6, 5} },   {"w g pawn", {6, 6} },   {"w h pawn", {6, 7} }, 
        {"b a pawn", {1, 0} },   {"b b pawn", {1, 1} },   {"b c pawn", {1, 2} },   {"b d pawn", {1, 3} }, 
        {"b e pawn", {1, 4} },   {"b f pawn", {1, 5} },   {"b g pawn", {1, 6} },   {"b h pawn", {1, 7} },
        {"w a rook", {7, 0} },   {"w h rook", {7, 7} },   {"b a rook", {0, 0} },   {"b h rook", {0, 7} },
        {"w b knight", {7, 1} }, {"w g knight", {7, 6} }, {"b b knight", {0, 1} }, {"b g knight", {0, 6} }, 
        {"w c bishop", {7, 2} }, {"w f bishop", {7, 5} }, {"b c bishop", {0, 2} }, {"b f bishop", {0, 5} },
        {"w queen", {7, 3} },    {"b queen", {0, 3} },    {"w king", {7, 4} },     {"b king", {0, 4} }
    };
    
    // Slight delay before prompting a replay
    QTimer::singleShot(10, this, [this]() {
        info->promptGameMode(this);
    });
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

// Translate from QPoint to chess::Square
chess::Square ChessBoard::squareFromQt(QPoint q) {
    int row = q.y();  // Qt row
    int col = q.x();  // Qt column

    int chessRank = 7 - row;  // Convert Qt row to chess rank
    int chessFile = col;  // Column is already the same as the file (a=0 to h=7)
    int index = (chessRank * 8) + chessFile;

    return static_cast<chess::Square>(index);
}

// Translate from chess::Move to QPoint
QPoint ChessBoard::qtFromMove(chess::Move mv) {
    int index = mv.to().index();  // 0 to 63
    int rank = index / 8;  // 0 (rank 1) to 7 (rank 8)
    int file = index % 8;  // 0 = 'a', 7 = 'h'

    int qtRow = 7 - rank;  // Qt row: 0 = top, 7 = bottom
    int qtCol = file;  // Qt col = file

    return QPoint(qtCol, qtRow);
}

// Translate from chess::Square to QPoint
QPoint ChessBoard::qtFromSquare(chess::Square sq) {
    int index = sq.index();

    int rank = index / 8;
    int file = index % 8;

    int qtRow = 7 - rank;
    int qtCol = file;

    return QPoint(qtCol, qtRow);
}

QString ChessBoard::pieceTypeToQString(chess::PieceType pt) {
    if (pt == chess::PieceType::PAWN)        { return "pawn"; }
    else if (pt == chess::PieceType::KNIGHT) { return "knight"; }
    else if (pt == chess::PieceType::BISHOP) { return "bishop"; }
    else if (pt == chess::PieceType::ROOK)   { return "rook"; }
    else if (pt == chess::PieceType::QUEEN)  { return "queen"; }
    else if (pt == chess::PieceType::KING)   { return "king"; }
    else                                     { return "";}
}