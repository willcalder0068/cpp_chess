#include "userinformation.h"
#include "promptdialog.h"
#include "mainwindow.h"
#include "runstockfish.h"
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <algorithm>

using namespace std;

// If the string has no beginning, it is empty. If not, build a trimmed substring
string UserInformation::trim(const string& str) {
    auto begin = str.find_first_not_of(" \t\r\n");
    auto end = str.find_last_not_of(" \t\r\n");
    return (begin == string::npos) ? "" : str.substr(begin, end - begin + 1);
}


UserInformation::UserInformation(int input, QWidget *parentWidget) {
    running = false;  // Engine has not been initialized in main, toggleOff to avoid setElo

    // We have already filtered erroneous inputs in main, so just check for 1 and 2
    if (input == 1)      { registerUser(parentWidget); }
    else if (input == 2) { loginUser(parentWidget); }
}

void UserInformation::registerUser(QWidget *parentWidget) {
    string password, passwordConfirm;  // Run it twice so the user is sure of their password
    auto users = loadUsers();  // Returns the map of all users (username, (salt, hash)) in the USERBASE_FILE

    // We loop until the user enters a valid input or exits
    while (true) {
        PromptDialog prompt("Create a username: ", parentWidget);
        prompt.followParent();  // Check for movement
        prompt.exec();  // Display dialog prompt
        username = trim(prompt.getInputText().toStdString());

        if (username == "")
            QMessageBox::warning(parentWidget, "Invalid Username", "Invalid. Try again.");  // Empty string causes retrieval errors when logging in
        else if (username == "0") { std::exit(0); }  // Terminate program
        else if (users.count(username))
            QMessageBox::warning(parentWidget, "Username Taken", "User already exists. Try again.");
        else { break; }  // Exit the loop (valid username)
    }

    while (true) {
        PromptDialog passPrompt("Create a password: ", parentWidget);
        passPrompt.followParent();  // Check for movement
        passPrompt.exec();  // Display dialog prompt
        password = trim(passPrompt.getInputText().toStdString());

        if (password == "0") { std::exit(0); }  // Terminate program
        else if (password == "" )
            QMessageBox::warning(parentWidget, "Invalid Password", "Invalid. Try again.");  // Empty string causes hashing errors
        else {
            PromptDialog confirmPrompt("Confirm password: ", parentWidget);  // Make sure the user knows their password
            confirmPrompt.followParent();  // Check for movement
            confirmPrompt.exec();  // Display dialog prompt
            passwordConfirm = trim(confirmPrompt.getInputText().toStdString());

            if (passwordConfirm == "0") { std::exit(0); }  // Terminate program
            else if (password == passwordConfirm) { break; }  // Exit the loop
            else
                QMessageBox::warning(parentWidget, "Mismatching Passwords", "Passwords do not match. Try again.");  // Re-loop
        }
    }

    string salt = generateSalt();  // Generate the random salt for our user
    string hashed = sha256(password + PEPPER + salt);  // Add the pepper for encryption and the salt for uniqueness
    saveUser(username, salt, hashed);  // Save the user to the database
    QMessageBox::information(parentWidget, "Success", "User registered.");

    gameMode = 2;  // There are no old games to review, so mark the gameMode as new game, then prompt for elo
    promptForElo(parentWidget);
}

void UserInformation::loginUser(QWidget *parentWidget) {
    string password;
    auto users = loadUsers();  // Returns the map of all users (username, (salt, hash))

    while (true) {
        PromptDialog userPrompt("Enter your username: ", parentWidget);
        userPrompt.followParent();  // Check for movement
        userPrompt.exec();  // Display dialog prompt
        username = trim(userPrompt.getInputText().toStdString());

        if (username == "")
            QMessageBox::warning(parentWidget, "Invalid Username", "Invalid. Try again.");  // Empty string causes retrieval errors when logging in
        else if (username == "0") { std::exit(0); }  // Terminate program
        else if (!users.count(username))
            QMessageBox::warning(parentWidget, "Not Found", "User does not exist. Try again.");
        else { break; }  // Exit the loop (valid username)
    }

    while (true) {
        PromptDialog passPrompt("Enter your password: ", parentWidget);
        passPrompt.followParent();  // Check for movement
        passPrompt.exec();  // Display dialog prompt
        password = trim(passPrompt.getInputText().toStdString());

        bool validPassword = true;

        if (password == "0") { std::exit(0); }  // Terminate program
        else if (password == "") {
            QMessageBox::warning(parentWidget, "Invalid password", "Invalid password. Try again.");  // Empty string causes hashing errors
            validPassword = false;  // No break, must re-loop
        }

        if (validPassword) {
            auto [salt, storedHash] = users[username];  // Iniialize variables salt and storedHash with the values in the user map
            string inputHash = sha256(password + PEPPER + salt);  // Get the hash generated from the entered password

            if (inputHash == storedHash) {
                QMessageBox::information(parentWidget, "Success", "Login successful.");
                break;  // Exit the loop
            } else
                QMessageBox::warning(parentWidget, "Incorrect", "Wrong password. Try again.");
        }   
    }

    writeQuit(username);  // Check if the user ended the last game with an unnatural exit
    promptGameMode(parentWidget);
}

void UserInformation::promptGameMode(QWidget* parentWidget) {
    while (true) {
        PromptDialog gameModePrompt("Press 1 to review or 2 to start a new game: ", parentWidget);
        gameModePrompt.followParent();  // Check for movement
        gameModePrompt.exec();  // Display dialog prompt
        string sMode = trim(gameModePrompt.getInputText().toStdString());

        bool isInt = true;
        gameMode = -1;  // Reset gameMode to avoid using stale values

        // Try to convert to int; if the user typed a non int, fall through to the QMessageBox warning
        try {
            gameMode = std::stoi(sMode);
        } catch (...) { isInt = false; }

        if (isInt) {
            if (gameMode == 1 || gameMode == 2) { break; }  // Exit the loop before calling promptForElo to avoid runtime errors
            else if (gameMode == 0) { std::exit(0); }  // Terminate program
        }
        QMessageBox::warning(parentWidget, "Error", "Not a valid game mode. Try again.");
    }

    if (gameMode == 1)      { promptForReview(parentWidget); }
    else if (gameMode == 2) { promptForElo(parentWidget); }
}

void UserInformation::promptForReview(QWidget* parentWidget) {
    // Make sure file can be opened
    ifstream in(USER_FILE_ROOT + username);
    if (!in.is_open()) {
        cerr << "Failed to open file for writing upon checking database: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        // Clear our previous containers
        while (!forwardMoves.empty()) { forwardMoves.pop(); }
        while (!backwardMoves.empty()) { backwardMoves.pop(); }
        sReviewInfo.clear();

        vector<string> reviewMovesList, prevGameInfo;
        int prevGamesCount = 0;
        string line;  // Container to read from the users file
        while (getline(in, line)) {
            istringstream iss1(line);  // Create an input string stream from the line
            string token1, token2;

            // Only true if the line contains '(', so only our games are tokenized (with the moves before '(' in token1, and the info in token2)
            if (getline(iss1, token1, '(') && getline(iss1, token2)) {
                // Make sure the line starts with ">>"
                if (token1.rfind(">>", 0) == 0) {
                    size_t p = token1.find_first_not_of("> "); // skip '>' and spaces
                    token1 = (p == string::npos) ? "" : token1.substr(p);
                }
                reviewMovesList.push_back(trim(token1));  // Store moves for each game in the vector
                prevGameInfo.push_back(trim(token2));  // Store the game info for each game in another
                prevGamesCount++;  // Incremenet the games count for the user prompt
            }
        }
        // In case the user has nothing to review
        if (prevGamesCount == 0) {
            QMessageBox::information(parentWidget, "No previous games", "Must play a new game.");
            promptForElo(parentWidget);
        }

        int gameNum;
        while (true) {
            PromptDialog reviewGamePrompt("Enter the game number for review [1 - " + QString::fromStdString(to_string(prevGamesCount)) + "]: ", parentWidget);
            reviewGamePrompt.followParent();  // Check for movement
            reviewGamePrompt.exec();  // Display dialog prompt
            string sGame = trim(reviewGamePrompt.getInputText().toStdString());

            if (sGame == "0") { std::exit(0); }  // Terminate program

            bool isInt = true;
            gameNum = -1;  // Reset gameNum to avoid using stale values

            // Try to convert to int; if the user typed a non int, fall through to the QMessageBox warning
            try {
                gameNum = std::stoi(sGame);
            } catch(...) { isInt = false; }

            if (isInt) {
                if (gameNum >= 1 && gameNum <= prevGamesCount) {
                    QMessageBox::information(parentWidget, "Game Found",  QString("Reviewing game %1").arg(gameNum));
                    break;  // Exit the loop
                }
            }

            QMessageBox::warning(parentWidget, "Error", "Not a valid game for review. Try again.");
        }

        vector<string> reviewMoves = {};
        string sReviewMoves = reviewMovesList[gameNum - 1];  // fill sReviewMoves with the moves from the chosen game
        istringstream iss2(sReviewMoves);  // Create an input string from the line
        string token;
        while (getline(iss2, token, ',')) {  // Split for moves at the commas
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end   = token.find_last_not_of(" \t\r\n");
            if (start != string::npos)
                reviewMoves.push_back(token.substr(start, end - start + 1));  // Store each move of the chosen game (whitespaces trimmed)
        }
        // Add moves to the stack from back to front (ends with the first move on top)
        while (!(reviewMoves.empty())) {
            forwardMoves.push(trim(reviewMoves.back()));
            reviewMoves.pop_back();
        }

        // Store the game information (used on the Gui display and the end message)
        sReviewInfo = QString::fromStdString(prevGameInfo[gameNum - 1]);
        if (sReviewInfo.contains("Quit")) { 
            isWhite = false;  // Placeholder
            elo = -1;  // Placeholder
        }
        else {
            if (sReviewInfo.contains("White")) { isWhite = true; }
            else { isWhite = false; }

            if (sReviewInfo.contains("Friend")) { elo = 1; }
            else
                elo = stoi(trim(sReviewInfo.toStdString().substr(sReviewInfo.size() - 9, 4)));
                // If its three digits, we strip the ending whitespace. If its four digits, we take the whole substring
        }
    }
}

void UserInformation::promptForElo(QWidget* parentWidget) {
    sReviewInfo = "tmp";  // Placeholder to avoid crashing in chessboard's paintEvent

    while (true) {
        // The user can play the computer with a certain elo, or they can play against a friend
        PromptDialog registerEloPrompt("Enter desired opponent elo [800-3000]: \n  (Press 1 to play a friend)", parentWidget);
        registerEloPrompt.followParent();  // Check for movement
        registerEloPrompt.exec();  // Display dialog prompt
        string sElo = trim(registerEloPrompt.getInputText().toStdString());

        bool isInt = true;
        elo = -1;  // Reset elo to avoid using stale values

        // Try to convert to int; if the user typed a non int, fall through to the QMessageBox warning
        try {
            elo = std::stoi(sElo);
        } catch(...) { isInt = false; }

        if (isInt) { 
            if (elo >= 800 && elo <= 3000) { 

                if (elo == 1590) { elo = 3200; }

                // If the engine has been initiated in main, setElo (if not, elo will be set in main)
                if (running) { 
                    engine->markNewGame();
                    engine->setElo(elo); 
                }

                QMessageBox::information(parentWidget, "Successful Matchup", "Playing opponent with elo " + QString::number(elo));
                break;  // Exit the loop
            }
            else if (elo == 1) {
                QMessageBox::information(parentWidget, "Successful Matchup", "Playing against a friend.");
                break;  // Exit the loop
            }
            else if (elo == 0) { std::exit(0); }  // Terminate
        }
        QMessageBox::warning(parentWidget, "Error", "Elo must be between 800 and 3000. Try again.");
    }

    // Give the user a color (white or black)
    random_device colorSeed;
    mt19937 gen(colorSeed());
    uniform_int_distribution<> colorDis(0, 1);  // Pick between 0 and 1
    if (colorDis(gen) == 0) { 
        isWhite = true; 
        computerTurn = false;
    }
    else { 
        isWhite = false; 
        computerTurn = true;
    }

    // Initialize to false; toggled to change what we write to the file
    castleWQ = false;
    castleWK = false;
    castleBQ = false;
    castleBK = false;
}

string UserInformation::generateSalt(int length) {
    static const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";  // Establish an array of characters
    random_device seed;  // Make a seed for our randomness

    mt19937 gen(seed());  // Initialize the RNG (mt19937 is the RNG)
    uniform_int_distribution<> dis(0, 35);  // Set unifrom distribution between all of the characters (from 0 to 35, the whole range)

    string salt;
    for (int i = 0; i < length; ++i)
        salt += chars[dis(gen)];  // Append one of the random characters to the salt

    return salt;  // Return the randomly generated salt; extremely unlikely to have a repeat value
}

// Compute SHA-256 hash
string UserInformation::sha256(const string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];  // declares an array called 'hash' of size 32 (SHA256 digest length) holding unsigned chars

    SHA256((const unsigned char*)input.c_str(), input.size(), hash);  // Hashes the input string (taken as a character array); puts the 32 char result in 'hash'

    stringstream ss;
    for (unsigned char c : hash)
        ss << hex << setw(2) << setfill('0') << (int) c;  // Char occupies one byte; two hex values can be stored in one byte (hence width is 2)

    return ss.str();  // Returns the hash code as a string (not a stringstream)
}

// Return a map containing user data (username, (salt, hash))
map<string, pair<string, string>> UserInformation::loadUsers() {
    ifstream in(USERBASE_FILE);
    if (!in.is_open()) {
        cerr << "Failed to open file for writing upon checking database: " << USERBASE_FILE << endl;
        std::exit(0);
        return {};
    }
    else {
        map<string, pair<string, string>> users;
        string line, username, salt, hash;

        while (getline(in, line)) {  // Reads line from the in file
            istringstream iss(line);  // Create an input string stream from the line
            iss >> username >> salt >> hash;  // Split the string into 3 tokens (separated by spaces)
            users[username] = {salt, hash};  // Add a map coordinate with the user, their salt, and their hash
        }
        return users;
    }
}

// Make a new file to record for our new user as well as adding their information to the database
void UserInformation::saveUser(const string& username, const string& salt, const string& hash) {
    ofstream out1(USERBASE_FILE, ios::app);  // Save the user information to the user database file
    if (!out1.is_open()) {
        cerr << "Failed to open file for writing upon saving to database: " << USERBASE_FILE << endl;
        std::exit(0);
        return;
    }
    else {
        out1 << username << " " << salt << " " << hash << "\n";  // Separate the tokens by spaces; separate the users by newlines
    }

    ofstream out2(USER_FILE_ROOT + username, ios::app);  // Print the username to the users personal file; holds the result of every game they have played
    if (!out2.is_open()) {
        std::cerr << "Failed to open file for writing upon creating personal file: " << USER_FILE_ROOT + username << std::endl;
        std::exit(0);
        return;
    }
    else {
        out2 << username << "\n---------------------------------------------------------------------\n>> ";  // Print header
    }
}

void UserInformation::writeMove(const string& username, const string& move) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon move: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        out << move << ", ";  // Separate each move with a comma
    }
}

void UserInformation::writeExit(const string& username) {
    std::string line, lastLine;

    ifstream in(USER_FILE_ROOT + username);
    if (!in.is_open()) {
        cerr << "Failed to open file for writing upon exit: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        while (getline(in, line)) {
            if (!line.empty()) { lastLine = line; }
        }
    }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon exit: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else if (lastLine != ">> ") {
        string vsElo = to_string(elo);
        if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
        else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }

        if (isWhite)
            out << "(Undetermined | White User Exit" << vsElo;
        else 
            out << "(Undetermined | Black User Exit" << vsElo;
    } 
}

void UserInformation::writeQuit(const string& username) {
    std::string line, lastLine;

    ifstream in(USER_FILE_ROOT + username);
    if (!in.is_open()) {
        cerr << "Failed to open file for writing upon quit: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        while (getline(in, line)) {
            if (!line.empty()) { lastLine = line; }
        }
    }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon quit: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else if (lastLine != ">> ") {
        out << "(Undetermined | User Quit)\n>> ";
    } 
}

void UserInformation::writeCM(const string& username, const string& loser) {
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon checmate: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        if (loser == "b" && isWhite)
            out << "(Checkmate | White User Win" << vsElo;
        else if (loser == "b" && !isWhite)
            out << "(Checkmate | Black User Loss" << vsElo;
        else if (loser == "w" && isWhite)
            out << "(Checkmate | White User Loss" << vsElo;
        else
            out << "(Checkmate | Black User Win" << vsElo;
    }
}

void UserInformation::writeStale(const string& username) {
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon stalemate: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        if (isWhite)
            out << "(Stalemate | White User Draw" << vsElo;
        else
            out << "(Stalemate | Black User Draw" << vsElo;
    }
}

void UserInformation::writeIN(const string& username) {
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon insufficient material: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        if (isWhite)
            out << "(Insufficient Material | White User Draw" << vsElo;
        else
            out << "(Insufficient Material | Black User Draw" << vsElo;
    }
}

void UserInformation::writeThree(const string& username) {
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }
    
    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon threefold repetition: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        if (isWhite)
            out << "(Threefold Repetition | White User Draw" << vsElo;
        else
            out << "(Threefold Repetition | Black User Draw" << vsElo;
    }
}

void UserInformation::writeFifty(const string& username) {
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else { vsElo = " | vs. " + vsElo + " Elo)\n>> "; }

    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        cerr << "Failed to open file for writing upon fifty move rule: " << USER_FILE_ROOT + username << endl;
        std::exit(0);
        return;
    }
    else {
        if (isWhite)
            out << "(Fifty Move Rule | White User Draw" << vsElo;
        else
            out << "(Fifty Move Rule | Black User Draw" << vsElo;
    }
}