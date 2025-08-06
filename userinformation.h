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

class UserInformation {
    public:
        UserInformation(int input, QWidget *parent);

        int elo;
        int gameMode;
        
    private:
        const std::string USER_FILE_ROOT = "C:/Users/wscal/OneDrive/Desktop/cpp/chess/userdata/userrecords/";  // Store individual file in user records folder
        const std::string USERBASE_FILE = "C:/Users/wscal/OneDrive/Desktop/cpp/chess/userdata/userbase.txt";  // Add user information to the user base
        const std::string PEPPER = "asdjkgb1458u79sdgkuh";

        std::string generateSalt(int length = 12);
        std::string sha256(const std::string& input);
        std::map<std::string, std::pair<std::string, std::string>> loadUsers();
        void saveUser(const std::string& username, const std::string& salt, const std::string& hash);

        void registerUser(QWidget *parentWidget);
        void loginUser(QWidget *parentWidget);
        void promptGameMode(QWidget *parentWidget);
        void promptForElo(QWidget *parentWidget);

        std::string trim(const std::string& str);
};

// A 'PEPPER' is an unknown string used to complicate encryption so that even if the database of hashes is breached, none of them will be able to be decoded.
// A 'SALT' is a random string that is used to ensure that every user has a unique hash, regardless of their password (perhaps not unique).

#endif