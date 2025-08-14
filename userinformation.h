#ifndef USERINFORMATION_H
#define USERINFORMATION_H

#include <iostream>  // Input and output; cin, cout, endl
#include <fstream>  // Enables file input and output; ifstream, ofstream
#include <sstream>  // Used for formatting strings like streams
#include <string>
#include <map>  // Gives us a sorted container which is implemented as a binary tree
#include <iomanip>
#include <openssl/sha.h>  // Uses lssl and lcrypto; if we dont compile the program by linking these libraries, this will throw an error
#include <random>
#include <cctype>  // Allows us to check character types
#include <algorithm>
#include <QWidget>
#include <stack>
#include "runstockfish.h"

class UserInformation {
    public:
        // From the prompt in main, direct the user to either login or register
        UserInformation(int input, QWidget *parent);

        // Public fields; will be accessed form chessboard after the event loop has started
        int elo, gameMode;  // Elo is used for opponent strength; gameMode is used to indictate if the user is playing or reviewing
        bool isWhite, running, computerTurn;  // User piece color, engine setElo bool, computerTurn bool
        std::string username;  // Username is needed to write to the individual file

        void promptGameMode(QWidget *parentWidget);  // Prompt the user to play or review

        // Used for review mode
        std::stack<std::string> forwardMoves;
        std::stack<std::string> backwardMoves;
        QString sReviewInfo;

        // Writing each move and outcome to the individuals file
        void writeMove(const std::string& username, const std::string& move);  // Move
        void writeQuit(const std::string& username);  // User exits window unnaturally
        void writeExit(const std::string& username);  // User exits window naturally
        void writeCM(const std::string& username, const std::string& loser);  // Checkmate
        void writeStale(const std::string& username);  // Stalemate
        void writeIN(const std::string& username);  // Insufficient material
        void writeThree(const std::string& username);  // Threefold repitition
        void writeFifty(const std::string& username);  // Fifty move rule

        StockfishEngine* engine = nullptr;
        const QString enginePath = "C:/Users/wscal/OneDrive/Desktop/cpp/chess/extern/stockfish/stockfish.exe";

        bool castleWK, castleBK, castleBQ, castleWQ;  // Alter file output on castle moves
        
    private:
        const std::string USER_FILE_ROOT = "C:/Users/wscal/OneDrive/Desktop/cpp/chess/userdata/userrecords/";  // Store individual file in user records folder
        const std::string USERBASE_FILE = "C:/Users/wscal/OneDrive/Desktop/cpp/chess/userdata/userbase.txt";  // Add user information to the user base
        const std::string PEPPER = "asdjkgb1458u79sdgkuh";

        void registerUser(QWidget *parentWidget);  // Register a new user
        void loginUser(QWidget *parentWidget);  // Login an existing user

        std::string generateSalt(int length = 12);
        std::string sha256(const std::string& input);  // Imported hash function; makes the password into an irreveversible hash for safe storage
        std::map<std::string, std::pair<std::string, std::string>> loadUsers();  // Loads from the userbase to check if the username is stored
        void saveUser(const std::string& username, const std::string& salt, const std::string& hash);  // Saves a new registered user to the userbase

        void promptForElo(QWidget *parentWidget);  // Prompt the user to play a friend (elo == 1) or to select opponent elo
        void promptForReview(QWidget* parentWidget);  // Review a game

        std::string trim(const std::string& str);  // Trim leading and trailing whitespace from input
};

// A 'PEPPER' is an unknown string used to complicate encryption so that even if the database of hashes is breached, none of them will be able to be decoded.
// A 'SALT' is a random string that is used to ensure that every user has a unique hash, regardless of their password (perhaps not unique).

#endif