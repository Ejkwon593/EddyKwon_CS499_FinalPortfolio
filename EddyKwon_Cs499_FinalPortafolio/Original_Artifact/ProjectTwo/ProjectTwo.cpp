// ProjectTwo.cpp
// CS300 – Advising Assistance Program
// Author: eddy kwon
// Date: 08/18/2025
// Description: Command-line course planner that loads a CSV, prints a sorted
// list of courses, and displays details (title + prerequisites) for a course.
// Notes: Single-file solution, no external headers or CSV parsers.

// ProjectTwo.cpp — CS300 Course Planner (single-file solution)
// -----------------------------------------------------------------------------
// This program loads Computer Science curriculum data from a CSV file and
// provides a command-line menu to:
//   1) Load the data structure from a file
//   2) Print an alphanumeric list of courses
//   3) Print details (title + prerequisites) for a specific course
//   9) Exit
//
// CSV format (per line):
//   courseNumber,courseTitle,prereq1,prereq2,... (0 or more prerequisites)
//
// Example:
//   CSCI100,Introduction to Computer Science
//   CSCI101,Introduction to Programming in C++,CSCI100
//
// The solution is self-contained (ONE .cpp file, no external headers).
// It follows best practices: clear names, comments, input validation,
// and robust normalization of course codes (handles BOM/NBSP/odd spaces).
//
// Compile: g++ -std=c++17 -O2 ProjectTwo.cpp -o planner
// Run:     ./planner
// -----------------------------------------------------------------------------

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------------
// Data model for a course
// -----------------------------------------------------------------------------
struct Course {
    std::string number;                 // e.g., CSCI200
    std::string title;                  // e.g., Data Structures
    std::vector<std::string> prereqs;   // e.g., {"CSCI101", "MATH201"}
};

// -----------------------------------------------------------------------------
// String helpers (trimming, BOM removal, canonicalization)
// -----------------------------------------------------------------------------

// Trim whitespace on the left
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
        [](unsigned char ch) { return !std::isspace(ch); }));
}

// Trim whitespace on the right
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
        [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// Trim both ends
static inline void trim(std::string& s) { ltrim(s); rtrim(s); }

// Remove a UTF-8 BOM if present at the beginning of a string
static inline void stripBOM(std::string& s) {
    if (s.size() >= 3 &&
        (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF) {
        s.erase(0, 3);
    }
}

// Canonicalize any course code or prerequisite code so that
// weird spaces (including NBSP), BOMs, punctuation, etc. cannot break lookups.
// We keep ONLY ASCII letters and digits and convert letters to UPPERCASE.
static std::string canonCode(std::string s) {
    stripBOM(s);
    trim(s);
    std::string out;
    out.reserve(s.size());
    for (unsigned char ch : s) {
        if (std::isalnum(ch)) {
            out.push_back((char)std::toupper(ch));
        }
        // all other characters are discarded on purpose
    }
    return out;
}

// Split a CSV line by commas (no quoted fields are required by this assignment)
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
// Loading: read CSV into an in-memory catalog (hash table)
// Using unordered_map gives O(1) average lookup by course code.
// -----------------------------------------------------------------------------
static bool loadCourses(const std::string& filename,
    std::unordered_map<std::string, Course>& catalog)
{
    std::ifstream fin(filename);
    if (!fin.is_open()) return false;

    catalog.clear();
    std::string line;
    size_t lineNum = 0;

    while (std::getline(fin, line)) {
        ++lineNum;

        // Skip empty/comment lines safely
        std::string check = line;
        stripBOM(check);
        trim(check);
        if (check.empty() || check[0] == '#') continue;

        // Split fields
        auto fields = splitCSV(line);
        if (fields.size() < 2) {
            std::cerr << "Warning line " << lineNum << ": not enough fields; skipping.\n";
            continue;
        }

        // Build a Course object
        Course c;
        c.number = canonCode(fields[0]);  // normalized key
        c.title = fields[1];

        for (size_t i = 2; i < fields.size(); ++i) {
            std::string p = canonCode(fields[i]);
            if (!p.empty()) c.prereqs.push_back(p);
        }

        if (c.number.empty()) {
            std::cerr << "Warning line " << lineNum
                << ": empty course code after normalization; skipping.\n";
            continue;
        }

        // If a duplicate code appears, last one wins (and silently replaces).
        catalog[c.number] = std::move(c);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Output: print sorted list of all courses (alphanumeric)
// -----------------------------------------------------------------------------
static void printCourseList(const std::unordered_map<std::string, Course>& catalog) {
    if (catalog.empty()) {
        std::cout << "No data loaded. Please load the data structure first.\n";
        return;
    }

    // Gather keys and sort them
    std::vector<std::string> keys; keys.reserve(catalog.size());
    for (const auto& kv : catalog) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());  // alphanumeric ascending

    std::cout << "Here is a sample schedule:\n";
    for (const auto& k : keys) {
        const auto& c = catalog.at(k);
        std::cout << c.number << ", " << c.title << "\n";
    }
}

// -----------------------------------------------------------------------------
// Output: print one course (title + prerequisites with titles when available)
// -----------------------------------------------------------------------------
static void printSingleCourse(const std::unordered_map<std::string, Course>& catalog,
    std::string userInputRaw)
{
    if (catalog.empty()) {
        std::cout << "No data loaded. Please load the data structure first.\n";
        return;
    }

    // Normalize the user’s query the same way we normalized the keys
    std::string key = canonCode(userInputRaw);
    auto it = catalog.find(key);
    if (it == catalog.end()) {
        std::cout << "Course " << userInputRaw << " was not found.\n";
        return;
    }

    const Course& c = it->second;
    std::cout << c.number << ", " << c.title << "\n";

    if (c.prereqs.empty()) {
        std::cout << "Prerequisites: None\n";
    }
    else {
        std::cout << "Prerequisites: ";
        for (size_t i = 0; i < c.prereqs.size(); ++i) {
            const std::string& p = c.prereqs[i];
            auto pit = catalog.find(p);
            if (pit != catalog.end()) {
                std::cout << pit->second.number << ", " << pit->second.title;
            }
            else {
                // If a prereq code wasn’t present as a full course in the CSV,
                // still show the code (title unavailable).
                std::cout << p << " (title unavailable)";
            }
            if (i + 1 < c.prereqs.size()) std::cout << "; ";
        }
        std::cout << "\n";
    }
}

// -----------------------------------------------------------------------------
// UI: simple menu loop with input validation
// -----------------------------------------------------------------------------
static void printMenu() {
    std::cout
        << "1. Load Data Structure.\n"
        << "2. Print Course List.\n"
        << "3. Print Course.\n"
        << "9. Exit\n";
}

int main() {
    std::unordered_map<std::string, Course> catalog;
    std::cout << "Welcome to the course planner.\n";

    bool running = true;
    while (running) {
        printMenu();
        std::cout << "What would you like to do? ";

        // Read the user’s menu selection as a string so we can validate it
        std::string choiceStr; std::getline(std::cin, choiceStr);
        trim(choiceStr);

        // If the user types a course code by mistake here, treat it as invalid.
        if (choiceStr.empty() || choiceStr.find_first_not_of("0123456789") != std::string::npos) {
            std::cout << (choiceStr.empty() ? "(empty)" : choiceStr) << " is not a valid option.\n";
            continue;
        }

        int choice = std::stoi(choiceStr);

        switch (choice) {
        case 1: {
            std::cout << "Enter the name of the data file (e.g., courses.csv): ";
            std::string filename; std::getline(std::cin, filename); trim(filename);

            if (filename.empty()) {
                std::cout << "No file name provided.\n";
                break;
            }

            if (loadCourses(filename, catalog)) {
                std::cout << "Loaded " << catalog.size() << " course(s) from \""
                    << filename << "\".\n";
            }
            else {
                std::cout << "Error: Could not open \"" << filename << "\".\n";
            }
            break;
        }

        case 2:
            printCourseList(catalog);
            break;

        case 3: {
            std::cout << "What course do you want to know about? ";
            std::string query; std::getline(std::cin, query);
            printSingleCourse(catalog, query);
            break;
        }

        case 9:
            std::cout << "Thank you for using the course planner!\n";
            running = false;
            break;

        default:
            std::cout << choice << " is not a valid option.\n";
            break;
        }
    }

    return 0;
}
