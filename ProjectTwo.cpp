#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// =============================================================
// ABCU Advising Program (BST Version)
// -------------------------------------------------------------
// Purpose:
//  - Load course data from a CSV file into a Binary Search Tree (BST)
//  - Validate file formatting and prerequisite references
//  - Print a sorted course list (in-order traversal)
//  - Print one course with its prerequisites
// =============================================================


// ==============================
// Course model
// ------------------------------
// Holds one course record from a single line of the input file.
// courseNumber: unique identifier used as the BST key (ex: "CSCI300")
// courseName: course title
// prerequisites: list of prerequisite course numbers (0..N items)
// ==============================
struct Course {
    string courseNumber;
    string courseName;
    vector<string> prerequisites;
};


// ==============================
// String helpers
// ==============================

// Removes leading and trailing whitespace from a string.
// This ensures " CSCI300 " becomes "CSCI300".
static inline string trim(const string& s) {
    size_t start = 0;
    while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

// Simple CSV split on commas.
// Note: This version assumes no commas inside quoted fields.
static vector<string> splitCSVSimple(const string& line) {
    vector<string> tokens;
    string token;
    stringstream ss(line);

    // Read each comma-separated token from the line
    while (getline(ss, token, ',')) {
        tokens.push_back(trim(token)); // trim each token for clean comparisons
    }
    return tokens;
}


// ==============================
// BST implementation (keyed by courseNumber)
// ==============================
//
// Stores Course objects in a BST using courseNumber as the sort key.
// In-order traversal prints courses in alphanumeric order automatically.
class CourseBST {
private:
    // Node stores one Course and pointers to left/right child nodes.
    struct Node {
        Course course;
        Node* left;
        Node* right;

        // Construct a node holding the provided course
        explicit Node(const Course& c) : course(c), left(nullptr), right(nullptr) {}
    };

    // Root pointer for the BST (nullptr when tree is empty)
    Node* root = nullptr;

    // Recursively frees memory in post-order (children before parent).
    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    // Recursive insert:
    //  - If subtree is empty, create a new node.
    //  - If courseNumber is smaller, go left.
    //  - If courseNumber is larger, go right.
    //  - If duplicate key, overwrite the existing course record.
    Node* insert(Node* node, const Course& course) {
        if (node == nullptr) {
            return new Node(course);
        }
        if (course.courseNumber < node->course.courseNumber) {
            node->left = insert(node->left, course);
        }
        else if (course.courseNumber > node->course.courseNumber) {
            node->right = insert(node->right, course);
        }
        else {
            // Duplicate courseNumber: overwrite existing (or ignore).
            node->course = course;
        }
        return node;
    }

    // Recursive BST search by courseNumber.
    const Course* search(Node* node, const string& courseNumber) const {
        if (node == nullptr) return nullptr;
        if (courseNumber == node->course.courseNumber) return &node->course;
        if (courseNumber < node->course.courseNumber) return search(node->left, courseNumber);
        return search(node->right, courseNumber);
    }

    // In-order traversal (left, current, right) prints in sorted order.
    void inOrderPrint(Node* node) const {
        if (!node) return;
        inOrderPrint(node->left);
        cout << node->course.courseNumber << ", " << node->course.courseName << endl;
        inOrderPrint(node->right);
    }

public:
    // Destructor: ensures all dynamically allocated nodes are freed.
    ~CourseBST() {
        destroy(root);
        root = nullptr;
    }

    // Clears the tree contents (used before re-loading a file).
    void Clear() {
        destroy(root);
        root = nullptr;
    }

    // Public insert wrapper.
    void Insert(const Course& course) {
        root = insert(root, course);
    }

    // Public search wrapper.
    const Course* Search(const string& courseNumber) const {
        return search(root, courseNumber);
    }

    // Prints all courses in alphanumeric order by courseNumber.
    void PrintInOrder() const {
        inOrderPrint(root);
    }
};


// ==============================
// Loading + validation
// ==============================
//
// Loads course data from filePath into the BST ONLY if the file is valid.
// Validation rules required by the assignment:
//  1) Each non-empty line must have at least 2 fields (courseNumber, courseName).
//  2) Every prerequisite listed must exist somewhere as a courseNumber in the file.
static bool LoadCoursesWithValidation(const string& filePath, CourseBST& tree) {
    // Attempt to open the input file
    ifstream infile(filePath);
    if (!infile.is_open()) {
        cout << "Error: Unable to open file: " << filePath << endl;
        return false;
    }

    // Temporary storage so we can validate prerequisites before inserting into BST
    vector<Course> tempCourses;

    // Collects all courseNumbers that appear in the file for fast prerequisite checks
    set<string> courseNumbers;

    string line;
    int lineNumber = 0;

    // Pass 1:
    //  - Read each line
    //  - Split into tokens
    //  - Validate at least 2 fields
    //  - Create Course objects
    //  - Collect courseNumbers
    while (getline(infile, line)) {
        ++lineNumber;
        line = trim(line);

        // Ignore blank lines
        if (line.empty()) {
            continue;
        }

        // Parse tokens from CSV line
        vector<string> tokens = splitCSVSimple(line);

        // Requirement: at least courseNumber + courseName
        if (tokens.size() < 2) {
            cout << "Format error on line " << lineNumber
                << ": must contain courseNumber and courseName" << endl;
            return false; // stop loading on format error
        }

        // Build a Course object from this line
        Course c;
        c.courseNumber = tokens[0];
        c.courseName = tokens[1];

        // Reject lines where required fields are empty after trimming
        if (c.courseNumber.empty() || c.courseName.empty()) {
            cout << "Format error on line " << lineNumber
                << ": courseNumber and courseName cannot be blank" << endl;
            return false;
        }

        // Remaining tokens (2..N) are prerequisite course numbers
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (!tokens[i].empty()) {
                c.prerequisites.push_back(tokens[i]);
            }
        }

        // Store course for later validation and loading
        courseNumbers.insert(c.courseNumber);
        tempCourses.push_back(c);
    }

    infile.close();

    // If nothing was read, file is not useful to the program
    if (tempCourses.empty()) {
        cout << "Error: File contained no course data." << endl;
        return false;
    }

    // Pass 2:
    // Validate that each prerequisite appears as a courseNumber in the file
    for (const Course& c : tempCourses) {
        for (const string& prereq : c.prerequisites) {
            if (courseNumbers.find(prereq) == courseNumbers.end()) {
                cout << "Prerequisite error: " << prereq << " is listed for "
                    << c.courseNumber << " but does not exist in the file." << endl;
                return false;
            }
        }
    }

    // If all validations passed, load into BST
    tree.Clear();
    for (const Course& c : tempCourses) {
        tree.Insert(c);
    }

    cout << "Courses loaded successfully (" << tempCourses.size() << " courses)." << endl;
    return true;
}


// ==============================
// Printing one course (Option 3)
// ==============================
//
// Prints a course's number and title.
// Then prints prerequisite course numbers and titles (if available).
static void PrintCourseAndPrereqs(const CourseBST& tree, const string& courseNumber) {
    // Find the requested course
    const Course* course = tree.Search(courseNumber);
    if (!course) {
        cout << "Course not found: " << courseNumber << endl;
        return;
    }

    // Print main course information
    cout << course->courseNumber << ", " << course->courseName << endl;

    // If no prerequisites, print "None"
    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None" << endl;
        return;
    }

    // Otherwise, list prerequisites (with titles when possible)
    cout << "Prerequisites:" << endl;
    for (const string& prereqNum : course->prerequisites) {
        const Course* prereqCourse = tree.Search(prereqNum);

        // Prereqs were validated during load, so prereqCourse should exist.
        // This fallback still prints something if the data changes.
        if (prereqCourse) {
            cout << "  " << prereqCourse->courseNumber << ", " << prereqCourse->courseName << endl;
        }
        else {
            cout << "  " << prereqNum << endl;
        }
    }
}


// ==============================
// Menu
// ==============================

// Displays the menu options required by the ABCU advising program.
static void PrintMenu() {
    cout << "\nMenu:\n";
    cout << "  1. Load Data Structure\n";
    cout << "  2. Print Course List\n";
    cout << "  3. Print Course\n";
    cout << "  9. Exit\n";
    cout << "Enter choice: ";
}


// ==============================
// Program Entry Point
// ==============================
int main(int argc, char* argv[]) {

    // Hardcoded input file path (per assignment instructions / user environment)
    // Note: Backslashes must be escaped in C++ string literals.
    string filePath = "C:\\Users\\nisha\\Documents\\SNHU\\CS-300\\CS 300 ABCU_Advising_Program_Input.csv";

    // BST that stores all loaded courses
    CourseBST coursesTree;

    // Tracks whether Option 1 has successfully loaded the data
    bool dataLoaded = false;

    int choice = 0;

    // Loop until user selects Exit (9)
    while (choice != 9) {
        PrintMenu();
        cin >> choice;

        // Input validation: handle non-integer menu input
        if (!cin) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        switch (choice) {

        case 1: {
            // Option 1: load and validate file, then store in BST
            dataLoaded = LoadCoursesWithValidation(filePath, coursesTree);
            break;
        }

        case 2: {
            // Option 2: print sorted course list (in-order traversal)
            if (!dataLoaded) {
                cout << "Please load the data first (Option 1)." << endl;
                break;
            }
            coursesTree.PrintInOrder();
            break;
        }

        case 3: {
            // Option 3: print a single course and its prerequisites
            if (!dataLoaded) {
                cout << "Please load the data first (Option 1)." << endl;
                break;
            }
            cout << "Enter course number: ";
            string courseNumber;
            cin >> courseNumber;
            PrintCourseAndPrereqs(coursesTree, courseNumber);
            break;
        }

        case 9:
            // Option 9: exit the program
            cout << "Thank you for using the course planner! Good bye." << endl;
            break;

        default:
            // Any other number is not a valid menu option
            cout << "Invalid option. Try again." << endl;
            break;
        }
    }

    return 0;
}