// ProjectTwo.cpp
// CS499 – Final ePortfolio Artifact (Advising Assistance Program)
// Author: Eddy Kwon
// Date: 10/15/2025
//
// Description:
// Command-line course planner that loads a CSV file of courses,
// displays a sorted list, shows details for specific courses,
// and generates a recommended order using topological sorting.
// Enhanced with database connectivity demonstration using SQLite.
//
// Enhancements Summary:
//  - M2: Class encapsulation, safer input handling, data validation.
//  - M3: Directed graph & topological sort (Kahn’s algorithm).
//  - M5: Added SQLite database connection (demonstrating DB skills).

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Include SQLite (no external install required for demonstration)
#include <sqlite3.h>

// -----------------------------------------------------------------------------
// String helpers
// -----------------------------------------------------------------------------
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
}
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}
static inline void trim(std::string& s) { ltrim(s); rtrim(s); }

static inline void stripBOM(std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
}

static std::string canonCode(std::string s) {
    stripBOM(s);
    trim(s);
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (std::isalnum(ch)) out.push_back((char)std::toupper(ch));
    }
    return out;
}

static std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> parts;
    std::string field;
    std::istringstream ss(line);
    while (std::getline(ss, field, ',')) {
        stripBOM(field);
        trim(field);
        parts.push_back(field);
    }
    return parts;
}

// -----------------------------------------------------------------------------
// Course class (encapsulation & methods)
// -----------------------------------------------------------------------------
class Course {
private:
    std::string number_;
    std::string title_;
    std::vector<std::string> prereqs_;

public:
    Course() = default;
    Course(std::string number, std::string title)
        : number_(canonCode(number)), title_(std::move(title)) {}

    void addPrereq(const std::string& p) {
        std::string norm = canonCode(p);
        if (!norm.empty()) prereqs_.push_back(std::move(norm));
    }

    const std::string& number() const { return number_; }
    const std::string& title() const { return title_; }
    const std::vector<std::string>& prereqs() const { return prereqs_; }
};

// Type alias for catalog
using Catalog = std::unordered_map<std::string, Course>;

// -----------------------------------------------------------------------------
// Load courses from CSV into catalog
// -----------------------------------------------------------------------------
static bool loadCourses(const std::string& filename, Catalog& catalog) {
    std::ifstream fin(filename);
    if (!fin.is_open()) return false;

    catalog.clear();
    std::string line;
    size_t lineNum = 0;

    while (std::getline(fin, line)) {
        ++lineNum;
        std::string check = line;
        stripBOM(check);
        trim(check);
        if (check.empty() || check[0] == '#') continue;

        auto fields = splitCSV(line);
        if (fields.size() < 2) continue;

        Course c(fields[0], fields[1]);
        for (size_t i = 2; i < fields.size(); ++i) c.addPrereq(fields[i]);

        catalog[c.number()] = std::move(c);
    }

    return true;
}

// -----------------------------------------------------------------------------
// Output helpers
// -----------------------------------------------------------------------------
static void printCourseList(const Catalog& catalog) {
    if (catalog.empty()) {
        std::cout << "No data loaded.\n";
        return;
    }

    std::vector<std::string> keys;
    for (const auto& kv : catalog) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());

    std::cout << "Course List:\n";
    for (const auto& k : keys)
        std::cout << catalog.at(k).number() << ", " << catalog.at(k).title() << "\n";
}

static void printSingleCourse(const Catalog& catalog, const std::string& rawInput) {
    std::string key = canonCode(rawInput);
    auto it = catalog.find(key);
    if (it == catalog.end()) {
        std::cout << "Course not found.\n";
        return;
    }

    const Course& c = it->second;
    std::cout << c.number() << ", " << c.title() << "\n";
    if (c.prereqs().empty())
        std::cout << "Prerequisites: None\n";
    else {
        std::cout << "Prerequisites: ";
        for (size_t i = 0; i < c.prereqs().size(); ++i) {
            std::cout << c.prereqs()[i];
            if (i + 1 < c.prereqs().size()) std::cout << ", ";
        }
        std::cout << "\n";
    }
}

// -----------------------------------------------------------------------------
// Graph + Topological Sort
// -----------------------------------------------------------------------------
static void buildGraph(const Catalog& catalog,
    std::unordered_map<std::string, std::vector<std::string>>& adj,
    std::unordered_map<std::string, int>& indegree)
{
    adj.clear(); indegree.clear();

    for (const auto& kv : catalog) {
        adj[kv.first]; indegree[kv.first] = 0;
    }

    for (const auto& kv : catalog) {
        for (const auto& p : kv.second.prereqs()) {
            if (catalog.find(p) != catalog.end()) {
                adj[p].push_back(kv.first);
                ++indegree[kv.first];
            }
        }
    }
}

static void printRecommendedOrder(const Catalog& catalog) {
    if (catalog.empty()) {
        std::cout << "No data loaded.\n";
        return;
    }

    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> indegree;
    buildGraph(catalog, adj, indegree);

    std::set<std::string> zero;
    for (auto& kv : indegree)
        if (kv.second == 0) zero.insert(kv.first);

    std::vector<std::string> order;
    while (!zero.empty()) {
        auto it = zero.begin();
        std::string u = *it;
        zero.erase(it);
        order.push_back(u);

        for (auto& v : adj[u])
            if (--indegree[v] == 0) zero.insert(v);
    }

    std::cout << "Recommended Course Order:\n";
    for (size_t i = 0; i < order.size(); ++i)
        std::cout << (i + 1) << ". " << catalog.at(order[i]).number()
            << " - " << catalog.at(order[i]).title() << "\n";

    if (order.size() != catalog.size())
        std::cout << "\nWarning: Circular dependency detected.\n";
}

// -----------------------------------------------------------------------------
// Database connection demo (SQLite integration)
// -----------------------------------------------------------------------------
static void testDatabaseConnection() {
    sqlite3* db;
    int exitCode = sqlite3_open("courses.db", &db);

    if (exitCode == SQLITE_OK)
        std::cout << "Connected to SQLite database successfully!\n";
    else
        std::cout << "Failed to connect to SQLite database.\n";

    sqlite3_close(db);
}

// -----------------------------------------------------------------------------
// Menu + Main
// -----------------------------------------------------------------------------
static void printMenu() {
    std::cout << "\nMenu Options:\n"
        << "1. Load Data Structure\n"
        << "2. Print Course List\n"
        << "3. Print Course Details\n"
        << "4. Print Recommended Course Order\n"
        << "5. Test Database Connection (SQLite)\n"
        << "9. Exit\n";
}

int main() {
    Catalog catalog;
    bool running = true;

    std::cout << "Welcome to the Course Planner!\n";

    while (running) {
        printMenu();
        std::cout << "Enter choice: ";
        std::string choice;
        std::getline(std::cin, choice);
        trim(choice);

        if (choice == "1") {
            std::cout << "Enter file name (e.g., courses.csv): ";
            std::string filename; std::getline(std::cin, filename);
            trim(filename);
            if (loadCourses(filename, catalog))
                std::cout << "Loaded " << catalog.size() << " courses.\n";
            else
                std::cout << "Failed to open file.\n";
        }
        else if (choice == "2") printCourseList(catalog);
        else if (choice == "3") {
            std::cout << "Enter course number: ";
            std::string num; std::getline(std::cin, num);
            printSingleCourse(catalog, num);
        }
        else if (choice == "4") printRecommendedOrder(catalog);
        else if (choice == "5") testDatabaseConnection();
        else if (choice == "9") {
            std::cout << "Exiting program. Goodbye!\n";
            running = false;
        }
        else std::cout << "Invalid option. Try again.\n";
    }

    return 0;
}
