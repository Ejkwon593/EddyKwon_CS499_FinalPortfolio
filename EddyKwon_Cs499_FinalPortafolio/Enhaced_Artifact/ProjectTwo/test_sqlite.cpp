/*
#include "sqlite3.h"
#include <iostream>
using namespace std;

int main() {
    sqlite3* db;
    int exit = 0;

    exit = sqlite3_open("test_database.db", &db);
    if (exit) {
        cout << "Error opening database: " << sqlite3_errmsg(db) << endl;
    } else {
        cout << "Database opened successfully!" << endl;
    }

    sqlite3_close(db);
    return 0;
}
*/