#include "chessboard.h"
#include "promptdialog.h"
#include "runstockfish.h"
#include <QPainter>
#include <QMouseEvent>
#include <QString>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <algorithm>
#include <stack>

// Translate from QPoint to chess::Square
chess::Square ChessBoard::squareFromQt(QPoint q) {
    int chessRank = 7 - q.y();  // Convert Qt row to chess rank
    int chessFile = q.x();  // Qt column is already the same as the file (a=0 to h=7)
    int index = (chessRank * 8) + chessFile;  // Squares indexed from 0 to 63

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
    int index = sq.index();  // 0 to 63
    int rank = index / 8;  // 0 (rank 1) to 7 (rank 8)
    int file = index % 8;  // 0 = 'a', 7 = 'h'

    int qtRow = 7 - rank;  // Qt row: 0 = top, 7 = bottom
    int qtCol = file;  // Qt col = file

    return QPoint(qtCol, qtRow);
}

// Translate from chess::PieceType to QString
QString ChessBoard::pieceTypeToQString(chess::PieceType pt) {
    if (pt == chess::PieceType::PAWN)        { return "pawn"; }
    else if (pt == chess::PieceType::KNIGHT) { return "knight"; }
    else if (pt == chess::PieceType::BISHOP) { return "bishop"; }
    else if (pt == chess::PieceType::ROOK)   { return "rook"; }
    else if (pt == chess::PieceType::QUEEN)  { return "queen"; }
    else if (pt == chess::PieceType::KING)   { return "king"; }
    else                                     { return "";}  // Empty square
}

// Get the QPoint from the guiToFen map given its QString (eg: "e4")
QPoint ChessBoard::fenToGui(const QHash<QPoint, QString> &map, const QString &square) {
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        if (it.value() == square)
            return it.key();
    }
    return QPoint(-1, -1); // not found
}

// Move a piece on the Gui from one QPoint to another
void ChessBoard::movePiece(QMap<QString, guiPiece>& boardGui, const QPoint& from,const QPoint& to) {
    QString movingPiece;

    // Find the piece at 'from'
    for (auto it = boardGui.cbegin(); it != boardGui.cend(); ++it) {
        if (it.value().row == from.y() && it.value().col == from.x()) {
            movingPiece = it.key();
            break;
        }
    }

    if (movingPiece.isEmpty()) {
        qWarning("No piece at starting position");
        return;
    }

    // Remove any piece already at 'to' (captured)
    for (auto it = boardGui.begin(); it != boardGui.end(); ) {
        if (it.value().row == to.y() && it.value().col == to.x())
            it = boardGui.erase(it);
        else
            ++it;
    }

    // Update the moving piece's coordinates
    boardGui[movingPiece] = {to.y(), to.x()};
}

// Find a key in boardGui by the row and col
QString ChessBoard::findKeyByCoords(const QMap<QString, guiPiece>& map, int row, int col) {
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (it.value().row == row && it.value().col == col)
            return it.key();
    }
    return QString(); // empty if not found
}


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

        // Starting board Gui
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
QSize ChessBoard::sizeHint() const { return QSize(8 * squareSize, 10 * squareSize); }

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
        if (info->gameMode == 2 || !(info->sReviewInfo.contains("Quit"))) {
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
        // If the last game was Quit, we don't have elo or color
        else if (info->sReviewInfo.contains("Quit")) {
            painter.setPen(Qt::black);
            painter.drawText(380, 500, QString::fromStdString("versus [unknown]"));
            painter.drawText(30, 500, QString::fromStdString("user is [unknown]"));
        }
        // If we are reviewing, indicate how to go to the previous / next move
        if (info->gameMode == 1) {
            painter.setPen(QPen(Qt::black, 2)); // thickness 2
            painter.drawLine(240, 490, 240, 560);

            QFont font = painter.font();
            font.setBold(true);

            painter.drawText(214, 528, "prev");
            painter.drawText(246, 528, "next");
        }
    }
}

// Called whenever the player clicks the screen
void ChessBoard::mousePressEvent(QMouseEvent *event) {
    // Coordinates divided by square size gives the clicked position in terms of squares, not pixels
    int row = event->pos().y() / squareSize;
    int col = event->pos().x() / squareSize;
    selectedSquare = QPoint(col, row);

    if (!info) { mousePressGame(selectedSquare); }  // Game not initialized, fall through
    else {
        if (info->gameMode == 1) { mousePressReview(col); }
        else                     { mousePressGame(selectedSquare); }
    }
}

void ChessBoard::mousePressReview(int col) {
    possiblePieceMoves.clear();  // Make sure no highlights remain

    // Left side click, previous move
    if (col < 4 && !(info->backwardMoves.empty())) {
        info->forwardMoves.push(info->backwardMoves.top());  // Put the reverse move on top of the forward stack
        std::string currMove = info->backwardMoves.top();
        info->backwardMoves.pop();  // Take move off of the stack

        // Inverse direction, we are going back to where the piece was before
        QPoint to = fenToGui(guiToFen, QString::fromStdString(currMove.substr(0, 2)));
        QPoint from = fenToGui(guiToFen, QString::fromStdString(currMove.substr(currMove.size() - 2, 2)));

        QString key = findKeyByCoords(boardGui, from.y(), from.x());  // Get our piece type from boardGui key

        // Check for promotion to invert (if we are moving a pawn, reset the boardGui key to pawn from queen)
        if (currMove.substr(3, 4) == "pawn" && (currMove.substr(currMove.size() - 1, 1) == "1" || currMove.substr(currMove.size() - 1, 1) == "8")) {
            // Reset the pawn from a queen to a pawn
            boardGui[QString::fromStdString(key.toStdString().substr(0, 3) + " pawn")] = {from.y(), from.x()};
            // Remove the queen
            boardGui.remove(key);
        }

        // Check for castle. If we are moving from e to c or g with a king, then we must invert the castle
        if (currMove.substr(0, 2) == "e1" && currMove.substr(currMove.size() - 2, 2) == "c1" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(3, 7), QPoint(0, 7));
            movePiece(boardGui, from, to);
            update();  // Call paintEvent
        }
        else if (currMove.substr(0, 2) == "e1" && currMove.substr(currMove.size() - 2, 2) == "g1" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(5, 7), QPoint(7, 7));
            movePiece(boardGui, from, to);
            update();  // Call paintEvent
        }
        else if (currMove.substr(0, 2) == "e8" && currMove.substr(currMove.size() - 2, 2) == "c8" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(3, 0), QPoint(0, 0));
            movePiece(boardGui, from, to);
            update();  // Call paintEvent
        }
        else if (currMove.substr(0, 2) == "e8" && currMove.substr(currMove.size() - 2, 2) == "g8" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(5, 0), QPoint(7, 0));
            movePiece(boardGui, from, to);
            update();  // Call paintEvent
        }
        // If not a castle, check for a diagonal pawn move
        else if  (currMove.substr(3, 4) == "pawn"  &&
            (   (currMove.substr(0, 1) == "a" &&                                                    currMove.substr(currMove.size() - 2, 1) == "b")  ||
                (currMove.substr(0, 1) == "b" && (currMove.substr(currMove.size() - 2, 1) == "a" || currMove.substr(currMove.size() - 2, 1) == "c")) ||
                (currMove.substr(0, 1) == "c" && (currMove.substr(currMove.size() - 2, 1) == "b" || currMove.substr(currMove.size() - 2, 1) == "d")) ||
                (currMove.substr(0, 1) == "d" && (currMove.substr(currMove.size() - 2, 1) == "c" || currMove.substr(currMove.size() - 2, 1) == "e")) ||
                (currMove.substr(0, 1) == "e" && (currMove.substr(currMove.size() - 2, 1) == "d" || currMove.substr(currMove.size() - 2, 1) == "f")) ||
                (currMove.substr(0, 1) == "f" && (currMove.substr(currMove.size() - 2, 1) == "e" || currMove.substr(currMove.size() - 2, 1) == "g")) ||
                (currMove.substr(0, 1) == "g" && (currMove.substr(currMove.size() - 2, 1) == "f" || currMove.substr(currMove.size() - 2, 1) == "h")) ||
                (currMove.substr(0, 1) == "h" &&  currMove.substr(currMove.size() - 2, 1) == "g")
            )) 
        {
            // If it is an enPassant move, then we put the enemy pawn back
            if (!epBoolStack.empty() && epBoolStack.top()) {
                // If we got rid of a white pawn, put it back
                if (enPassantTakeStack.top().startsWith("w")) {
                    boardGui[enPassantTakeStack.top()] = {from.y() - 1, from.x()};
                }
                // If we got rid of a black pawn, put it back
                else {
                    boardGui[enPassantTakeStack.top()] = {from.y() + 1, from.x()};
                }
                // Pop from the top of the stack
                enPassantTakeStack.pop();
                epBoolStack.pop();  // Keep track of en passants in relative to other pawn captures
                movePiece(boardGui, from, to);
                update();  // Call paintEvent
            }
            // If it is a nonEnPassant move, then we put the enemy piece back
            else {
                movePiece(boardGui, from, to);

                // Add the taken piece back to the Gui
                boardGui[pawnTakeStack.top()] = {from.y(), from.x()};

                // Pop from the top of the stack
                pawnTakeStack.pop();
                epBoolStack.pop();  // Keep track of en passants in relative to other pawn captures
                update();  // Call paintEvent
            }
        }
        // Not a castle or a pawn take
        else {
            if (!nonPawnTakeStack.empty()) {
                const QString capturedKey = nonPawnTakeStack.top();
                nonPawnTakeStack.pop();

                // Reinsert the captured key onto the boardGui
                if (!capturedKey.isEmpty()) {
                    movePiece(boardGui, from, to);

                    boardGui[capturedKey] = { from.y(), from.x() };
                    update();
                }
                // If the key is empty, nothing was captured, hence we have nothing to return
                else {
                    movePiece(boardGui, from, to);
                    update();
                }
            }
            // If the stack is empty, nothing was captured, hence we have nothing to return
            else {
                movePiece(boardGui, from, to);
                update();
            }
        }
    }
    // Non-left side click + out of forward moves: end agame
    else if (info->forwardMoves.empty()) {
        if (info->sReviewInfo.contains("Quit") || info->sReviewInfo.contains("Undetermined")) { gameOverUN(); }
        if (info->sReviewInfo.contains("Stalemate"))                                          { gameOverStale(); }
        else if (info->sReviewInfo.contains("Insufficient Material"))                         { gameOverIN(); }
        else if (info->sReviewInfo.contains("Threefold Repetition"))                          { gameOverThree(); }
        else if (info->sReviewInfo.contains("Fifty Move Rule"))                               { gameOverFifty(); }
        else if ((info->sReviewInfo.contains("Win") && info->isWhite) || (info->sReviewInfo.contains("Loss") && !(info->isWhite))) {
            boardHard.setFen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");  // Set a FEN with black to move so white is declared winner
            gameOverCM();
        }
        else if ((info->sReviewInfo.contains("Loss") && info->isWhite) || (info->sReviewInfo.contains("Win") && !(info->isWhite))) {
            boardHard.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");  // Set a FEN with white to move so black is declared winner
            gameOverCM();
        }
    }
    // Right side click, next move
    else if (col >= 4) {
        info->backwardMoves.push(info->forwardMoves.top());  // Put the current move on the top of the reverse stack
        std::string currMove = info->forwardMoves.top();
        info->forwardMoves.pop();  // Take the move off of the stack

        // Eg: "e2e4" becomes from e2, to e4
        QPoint from = fenToGui(guiToFen, QString::fromStdString(currMove.substr(0, 2)));
        QPoint to = fenToGui(guiToFen, QString::fromStdString(currMove.substr(currMove.size() - 2, 2)));

        // Check for castle. If we are moving from e to c or g with a king, we must castle
        if (currMove.substr(0, 2) == "e1" && currMove.substr(currMove.size() - 2, 2) == "c1" && QString::fromStdString(currMove).contains("king")) {
            // Change the rook placement (the king will be moved below)
            movePiece(boardGui, QPoint(0, 7), QPoint(3, 7));
        }
        else if (currMove.substr(0, 2) == "e1" && currMove.substr(currMove.size() - 2, 2) == "g1" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(7, 7), QPoint(5, 7));
        }
        else if (currMove.substr(0, 2) == "e8" && currMove.substr(currMove.size() - 2, 2) == "c8" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(0, 0), QPoint(3, 0));
        }
        else if (currMove.substr(0, 2) == "e8" && currMove.substr(currMove.size() - 2, 2) == "g8" && QString::fromStdString(currMove).contains("king")) {
            movePiece(boardGui, QPoint(7, 0), QPoint(5, 0));
        }
        // If not a castle, check for a diagonally moving pawn
        else if  (currMove.substr(3, 4) == "pawn"  &&  
            (   (currMove.substr(0, 1) == "a" &&                                                    currMove.substr(currMove.size() - 2, 1) == "b")  ||
                (currMove.substr(0, 1) == "b" && (currMove.substr(currMove.size() - 2, 1) == "a" || currMove.substr(currMove.size() - 2, 1) == "c")) ||
                (currMove.substr(0, 1) == "c" && (currMove.substr(currMove.size() - 2, 1) == "b" || currMove.substr(currMove.size() - 2, 1) == "d")) ||
                (currMove.substr(0, 1) == "d" && (currMove.substr(currMove.size() - 2, 1) == "c" || currMove.substr(currMove.size() - 2, 1) == "e")) ||
                (currMove.substr(0, 1) == "e" && (currMove.substr(currMove.size() - 2, 1) == "d" || currMove.substr(currMove.size() - 2, 1) == "f")) ||
                (currMove.substr(0, 1) == "f" && (currMove.substr(currMove.size() - 2, 1) == "e" || currMove.substr(currMove.size() - 2, 1) == "g")) ||
                (currMove.substr(0, 1) == "g" && (currMove.substr(currMove.size() - 2, 1) == "f" || currMove.substr(currMove.size() - 2, 1) == "h")) ||
                (currMove.substr(0, 1) == "h" &&  currMove.substr(currMove.size() - 2, 1) == "g")
            )) 
        {
            // Check if we directly took any piece
            bool found = false;
            for (auto it = boardGui.cbegin(); it != boardGui.cend(); ++it) {
                if (it.value().row == to.y() && it.value().col == to.x()) {
                    found = true;
                    break;
                }
            }
            // If there is nothing on the key to where it moves (then we have an en passant)
            if (!found) {
                // White moves "up" (Qt y decreases). Black moves "down" (Qt y increases).
                const bool moverIsWhite = (to.y() < from.y());
                // Correctly assign the removal square
                const int capRow = moverIsWhite ? (to.y() + 1) : (to.y() - 1);
                const int capCol = to.x();

                QString capturedKey;
                if (capRow >= 0 && capRow < 8) {
                    capturedKey = findKeyByCoords(boardGui, capRow, capCol);

                    // Remove the corresponding key
                    if (!capturedKey.isEmpty()) { boardGui.remove(capturedKey); } 
                }

                epBoolStack.push(true);  // Keep track of en passants in relative to other pawn captures
                enPassantTakeStack.push(capturedKey);
            }
            // If not, we need to push to removed piece to the stack and mark it as nonEnPassant
            else {
                epBoolStack.push(false);  // Keep track of en passants in relative to other pawn captures
                pawnTakeStack.push(findKeyByCoords(boardGui, to.y(), to.x()));
            }
        }
        // Not a castle or a pawn take; push to the nonPawnTakeStack (empty string pushed if no pieces taken)
        else {
            nonPawnTakeStack.push(findKeyByCoords(boardGui, to.y(), to.x()));
        }

        // Check for promotion
        if (currMove.substr(3, 4) == "pawn" && (currMove.substr(currMove.size() - 1, 1) == "1" || currMove.substr(currMove.size() - 1, 1) == "8")) {
            QString key = findKeyByCoords(boardGui, from.y(), from.x());
            // Swap the pawn for a queen
            boardGui[QString::fromStdString(key.toStdString().substr(0, 3) + " queen")] = {from.y(), from.x()};
            // Remove the old pawn
            boardGui.remove(key);
        }

        movePiece(boardGui, from, to);
        update();  // Call paintEvent
    }
}

void ChessBoard::mousePressGame(QPoint selectedSquare) {
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
            threeBool = false;  // Reset toggle
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

    if (info->elo != 1) {
        if (info->computerTurn) {
            // Get the best move from stockfish; returns a four char string (eg: "e2e4")
            QString move = info->engine->bestMove(QString::fromStdString(boardHard.getFen()), 5000);
            // Hijack pieceSqare and selectedSquare with the computer decision (translate from uci to QPoint)
            pieceSquare = fenToGui(guiToFen, move.mid(0,2));
            selectedSquare = fenToGui(guiToFen, move.mid(2,2));

            // Enable the computer to castle by correcting for chess.hpp selectedSquare location (on the rook)
            if ((guiToFen[pieceSquare].toStdString().substr(0,2) == "e1" || guiToFen[pieceSquare].toStdString().substr(0,2) == "e8") &&
                 boardHard.at(squareFromQt(pieceSquare)).type() == chess::PieceType::KING && guiToFen[selectedSquare].toStdString().substr(0,1) == "g")
            {
                selectedSquare = QPoint(selectedSquare.x() + 1, selectedSquare.y());
            }
            else if ((guiToFen[pieceSquare].toStdString().substr(0,2) == "e1" || guiToFen[pieceSquare].toStdString().substr(0,2) == "e8") &&
                      boardHard.at(squareFromQt(pieceSquare)) == chess::PieceType::KING && guiToFen[selectedSquare].toStdString().substr(0,1) == "c")
            {
                selectedSquare = QPoint(selectedSquare.x() - 2, selectedSquare.y());
            }

            possibleComputerMoves.clear();  // So the next select doesn't retain the old squares
            chess::Square currSquare = squareFromQt(pieceSquare);
            for (const auto& move : legalMoves) {
                if (move.from() == currSquare) {
                    possibleComputerMoves.push_back(qtFromMove(move));
                }
            }
        }
    }
    else { 
        info->computerTurn = false; // Mark as false to avoid instant mousePressGame callback
    }

    // If the click was on the board
    if (selectedSquare.x() >= 0 && selectedSquare.x() <=7 && selectedSquare.y() >= 0 && selectedSquare.y() <= 7) {
        chess::Square currSquare = squareFromQt(selectedSquare);  // Translate from QPoint to chess::Square

        // If the selected square is currently listed as possible move, then the user wants to move there
        if ((std::find(possiblePieceMoves.begin(), possiblePieceMoves.end(), selectedSquare) != possiblePieceMoves.end()) ||
            (std::find(possibleComputerMoves.begin(), possibleComputerMoves.end(), selectedSquare) != possibleComputerMoves.end())) {
            possiblePieceMoves.clear();  // So the next select doesn't retain the old highlighted squares
            possibleComputerMoves.clear();

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
                        removeSquare = QPoint(selectedSquare.x(), selectedSquare.y() + 1);  // Remove the piece in the square below
                    else  // If black is moving
                        removeSquare = QPoint(selectedSquare.x(), selectedSquare.y() - 1);  // Remove the piece in the square above
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

                // Check for castle; if a king moves from his starting position to file a, c, g, or h, then it must be a castle (a / c, g / h is stockfish vs chess.hpp format)
                if ((boardHard.at(squareFromQt(pieceSquare)) == chess::PieceType::KING && (guiToFen[pieceSquare].toStdString() == "e8" || guiToFen[pieceSquare].toStdString() == "e1") &&
                    (guiToFen[selectedSquare].toStdString().substr(0,1) == "h" || guiToFen[selectedSquare].toStdString().substr(0,1) == "a"  ))) {
                        if (selectedSquare.x() == 0) {  // Castle queenside
                            if (currFen.contains("w")) {
                                // Reassign the castling king and rook
                                boardGui["w a rook"] = {7, 3};
                                boardGui["w king"] = {7, 2};
                                // Flip the bool for review logging
                                info->castleWQ = true;
                            }
                            else {
                                boardGui["b a rook"] = {0, 3};
                                boardGui["b king"] = {0, 2};
                                info->castleBQ = true;
                            }
                        }
                        else {  // Castle kingside
                            if (currFen.contains("w")) {
                                boardGui["w h rook"] = {7, 5};
                                boardGui["w king"] = {7, 6};
                                info->castleWK = true;
                            }
                            else {
                                boardGui["b h rook"] = {0, 5};
                                boardGui["b king"] = {0, 6};
                                info->castleBK = true;
                            }
                        }
                }
                // Non castle move, update boardGui with the new move
                else {
                    boardGui[movingKey] = { selectedSquare.y(), selectedSquare.x() };
                }

                // Make the move on boardHard and write it to the user file
                for (const chess::Move& move : legalMoves) {
                    if (move.from() == squareFromQt(pieceSquare) && move.to() == squareFromQt(selectedSquare)) {
                        std::string fromIndex = guiToFen[pieceSquare].toStdString();
                        std::string name = pieceTypeToQString(boardHard.at(squareFromQt(pieceSquare)).type()).toStdString();
                        std::string toIndex = guiToFen[selectedSquare].toStdString();

                        // Retoggle our bools and change the file output to match stockfish format
                        if (info->castleWQ) {
                            info->castleWQ = false;
                            toIndex = "c1";
                        }
                        else if (info->castleBQ) {
                            info->castleBQ = false;
                            toIndex = "c8";
                        }
                        else if (info->castleWK) {
                            info->castleWK = false;
                            toIndex = "g1";
                        }
                        else if (info->castleBK) {
                            info->castleBK = false;
                            toIndex = "g8";
                        }

                        // Write our move to the user record
                        if (info && !info->username.empty())
                            info->writeMove(info->username, (fromIndex + " " + name + " " + toIndex));

                        boardHard.makeMove(move);  // Update boardHard with the new move

                        // Check for threefold repitition
                        QStringList threeMoves = QString::fromStdString(boardHard.getFen()).split(' ');
                        if (threeMoves[3] == "-") {  // If the move has en passant rights, it cannot be a repeat
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

                        if (info->elo != 1) {
                            if (info->computerTurn) {
                                info->computerTurn = false;
                                this->selectedSquare = selectedSquare;  // Highlight the computer move
                            }
                            else {
                                info->computerTurn = true;
                            }
                        }

                        break;
                    }
                }
                // Call the function again; do not wait for the click, just impose a short wait time
                if (info->computerTurn) { 
                    QTimer::singleShot(100, this, [this, selectedSquare]() {
                        mousePressGame(selectedSquare); 
                    });
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
    gameOver();
}

void ChessBoard::gameOverThree() {
    QMessageBox::information(this, "Threefold Repitition", "It's a Draw!");
    gameOver();
}

void ChessBoard::gameOverFifty() {
    QMessageBox::information(this, "Fifty Move Rule", "It's a Draw!");
    gameOver();
}

void ChessBoard::gameOverUN() {
    QMessageBox::information(this, "Game Over", "Erroneous Finish.");
    gameOver();
}

void ChessBoard::gameOver() {
    // Reset boardHard data
    boardHard = chess::Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    // Clear threefold trackers
    threeMoveStale.clear();
    threeMoveCastle.clear();

    // Clear review stacks
    while (!enPassantTakeStack.empty()) { enPassantTakeStack.pop(); }
    while (!pawnTakeStack.empty()) { pawnTakeStack.pop(); }
    while (!nonPawnTakeStack.empty()) { nonPawnTakeStack.pop(); }
    while (!epBoolStack.empty()) { epBoolStack.pop(); }

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