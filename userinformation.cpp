#include "userinformation.h"
#include "promptdialog.h"
#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <algorithm>


using namespace std;

// Constructor; we have already filtered for erroneous inputs
UserInformation::UserInformation(int input, QWidget *parentWidget) {
    if (input == 1)      { registerUser(parentWidget); }
    else if (input == 2) { loginUser(parentWidget); }
}

// Register a new user
void UserInformation::registerUser(QWidget *parentWidget) {
    string password, passwordConfirm;
    auto users = loadUsers();  // Returns the map of all users (username, (salt, hash))

    // We loop until the user enters a valid input or exits
    while (true) {
        PromptDialog prompt("Create a username: ", parentWidget);
        prompt.followParent();  // Check for movement
        prompt.exec();  // Display dialog prompt
        username = trim(prompt.getInputText().toStdString());  // Clean whitespace

        if (username == "")
            QMessageBox::warning(parentWidget, "Invalid Username", "Invalid. Try again.");
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
            QMessageBox::warning(parentWidget, "Invalid Password", "Invalid. Try again.");
        else {
            PromptDialog confirmPrompt("Confirm password: ", parentWidget);  // Make sure the user knows their password
            confirmPrompt.followParent();
            confirmPrompt.exec();
            passwordConfirm = trim(confirmPrompt.getInputText().toStdString());

            if (passwordConfirm == "0") { std::exit(0); }  // Terminate program
            else if (password == passwordConfirm) { break; }  // Exit the loop
            else
                QMessageBox::warning(parentWidget, "Mismatching Passwords", "Passwords do not match. Try again.");
        }
    }

    string salt = generateSalt();  // Generate the random salt for our user
    string hashed = sha256(password + PEPPER + salt);  // Add the pepper for encryption and the salt for uniqueness
    saveUser(username, salt, hashed);
    QMessageBox::information(parentWidget, "Success", "User registered.");

    gameMode = 2;  // There are no old games to review, so mark the gameMode as new game, then prompt for elo
    promptForElo(parentWidget);
}

// Log in the user
void UserInformation::loginUser(QWidget *parentWidget) {
    string password;
    auto users = loadUsers();  // Returns the map of all users (username, (salt, hash))

    while (true) {
        PromptDialog userPrompt("Enter your username: ", parentWidget);
        userPrompt.followParent();
        userPrompt.exec();
        username = trim(userPrompt.getInputText().toStdString());

        if (username == "")
            QMessageBox::warning(parentWidget, "Invalid Username", "Invalid. Try again.");
        else if (username == "0") { std::exit(0); }  // Terminate program
        else if (!users.count(username))
            QMessageBox::warning(parentWidget, "Not Found", "User does not exist. Try again.");
        else { break; }  // Exit the loop (valid username)
    }

    while (true) {
        PromptDialog passPrompt("Enter your password: ", parentWidget);
        passPrompt.followParent();
        passPrompt.exec();
        password = trim(passPrompt.getInputText().toStdString());

        bool validPassword = true;

        if (password == "0") { std::exit(0); }  // Terminate program
        else if (password == "") {
            QMessageBox::warning(parentWidget, "Invalid password", "Invalid password. Try again.");
            validPassword = false;
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

    writeQuit(username);
    promptGameMode(parentWidget);
}

void UserInformation::promptGameMode(QWidget* parentWidget) {
    while (true) {
        PromptDialog gameModePrompt("Press 1 to review or 2 to start a new game: ", parentWidget);
        gameModePrompt.followParent();
        gameModePrompt.exec();
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

    if (gameMode == 1) {
        elo = 1;  // Put impossible elo; we will be reviewing, not playing
        promptForReview(parentWidget);
    }
    if (gameMode == 2) { promptForElo(parentWidget); }
}

void UserInformation::promptForReview(QWidget* parentWidget) {

}

void UserInformation::promptForElo(QWidget* parentWidget) {
    while (true) {
        // The user can play the computer with a certain elo, or they can play against a friend
        PromptDialog registerEloPrompt("Enter desired opponent elo [500-2500]: \n  (Press 1 to play a friend)", parentWidget);
        registerEloPrompt.followParent();
        registerEloPrompt.exec();
        string sElo = trim(registerEloPrompt.getInputText().toStdString());  // Convert input from QString to std::string

        bool isInt = true;
        elo = -1;  // Reset elo to avoid using stale values

        // Try to convert to int; if the user typed a non int, fall through to the QMessageBox warning
        try {
            elo = std::stoi(sElo);
        } catch(...) { isInt = false; }

        if (isInt) { 
            if (elo >= 500 && elo <= 2500) { 
                if (elo == 1590) { elo = 2800; }  // Developer joke
                QMessageBox::information(parentWidget, "Successful Matchup", "Playing opponent with elo " + QString::number(elo));
                break;  // Exit the loop
            }
            else if (elo == 1) {
                QMessageBox::information(parentWidget, "Successful Matchup", "Playing against a friend.");
                break;  // Exit the loop
            }
            else if (elo == 0) { std::exit(0); }  // Terminate
        }
        QMessageBox::warning(parentWidget, "Error", "Elo must be between 500 and 2500. Try again.");
    }

    // Give the user a color (white or black)
    random_device colorSeed;
    mt19937 gen(colorSeed());
    uniform_int_distribution<> colorDis(0, 1);  // Pick between 0 and 1
    if (colorDis(gen) == 0) { isWhite = true; }
    else                    { isWhite = false; }
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
    map<string, pair<string, string>> users;
    ifstream in(USERBASE_FILE);  // In file stream

    string line, username, salt, hash;

    while (getline(in, line)) {  // Reads line from the in file
        istringstream iss(line);  // Create an input string stream from the line
        iss >> username >> salt >> hash;  // Split the string into 3 tokens (separated by spaces)
        users[username] = {salt, hash};  // Add a map coordinate with the user, their salt, and their hash
    }
    return users;
}

// Make a new file to record for our new user as well as adding their information to the database
void UserInformation::saveUser(const string& username, const string& salt, const string& hash) {
    ofstream out1(USERBASE_FILE, ios::app);  // Save the user information to the user database file
    out1 << username << " " << salt << " " << hash << "\n";  // Separate the tokens by spaces; separate the users by newlines

    ofstream out2(USER_FILE_ROOT + username, ios::app);  // Print the username to the users personal file; holds the result of every game they have played
    out2 << username << "\n---------------------------------------------------------------------\n>> ";  // Print header
}


void UserInformation::writeMove(const string& username, const string& move) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing: " << USER_FILE_ROOT + username << std::endl;
        return;
    }

    out << move << ", ";
}

void UserInformation::writeQuit(const string& username) {
    ifstream in(USER_FILE_ROOT + username, ios::app);
    std::string line, lastLine;

    while (std::getline(in, line)) {
        if (!line.empty()) { lastLine = line; }
    }

    ofstream out(USER_FILE_ROOT + username, ios::app);

    if (lastLine != ">> ") {
        out << "(Undetermined | User Quit)\n>> ";
    } 
}

void UserInformation::writeCM(const string& username, const string& loser) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else {
        vsElo = " | vs. " + vsElo + " Elo)\n>> ";
    }

    if (loser == "b" && isWhite)
        out << "(Checkmate | White User Win" << vsElo;
    else if (loser == "b" && !isWhite)
        out << "(Checkmate | Black User Loss" << vsElo;
    else if (loser == "w" && isWhite)
        out << "(Checkmate | White User Loss" << vsElo;
    else
        out << "(Checkmate | Black User Win" << vsElo;
}

void UserInformation::writeStale(const string& username) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else {
        vsElo = " | vs. " + vsElo + " Elo)\n>> ";
    }

    if (isWhite)
        out << "(Stalemate | White User Draw" << vsElo;
    else
        out << "(Stalemate | Black User Draw" << vsElo;
}

void UserInformation::writeIN(const string& username) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else {
        vsElo = " | vs. " + vsElo + " Elo)\n>> ";
    }

    if (isWhite)
        out << "(Insufficient Material | White User Draw" << vsElo;
    else
        out << "(Insufficient Material | Black User Draw" << vsElo;
}

void UserInformation::writeThree(const string& username) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else {
        vsElo = " | vs. " + vsElo + " Elo)\n>> ";
    }

    if (isWhite)
        out << "(Threefold Repitition | White User Draw" << vsElo;
    else
        out << "(Threefold Repitition | Black User Draw" << vsElo;
}

void UserInformation::writeFifty(const string& username) {
    ofstream out(USER_FILE_ROOT + username, ios::app);
    string vsElo = to_string(elo);
    if (vsElo == "1") { vsElo = " | vs. Friend)\n>> "; }
    else {
        vsElo = " | vs. " + vsElo + " Elo)\n>> ";
    }

    if (isWhite)
        out << "(Fifty Move Rule | White User Draw" << vsElo;
    else
        out << "(Fifty Move Rule | Black User Draw" << vsElo;
}

// Gets rid of the leading and trailing white spaces, returning the trimmed string
string UserInformation::trim(const string& str) {
    auto begin = str.find_first_not_of(" \t\r\n");
    auto end = str.find_last_not_of(" \t\r\n");
    // If the string has no beginning, it is empty. If not, build a trimmed substring
    return (begin == string::npos) ? "" : str.substr(begin, end - begin + 1);
}