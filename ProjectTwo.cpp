#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

// =============================================================
// ABCU Advising Program (Enhanced AVL Tree and Graph Version)
// -------------------------------------------------------------
// Purpose:
//  - Load and validate course data from a CSV file
//  - Store courses in a self-balancing AVL tree
//  - Provide efficient course searching and sorted traversal
//  - Represent prerequisite relationships using a directed graph
//  - Identify direct and indirect prerequisites using DFS
//  - Detect circular prerequisite dependencies
//  - Generate valid course sequences using topological sorting
// =============================================================


// ==============================
// Course model
// ------------------------------
// Holds one course record from a single line of the input file.
// courseNumber: unique identifier used as the AVL tree key (ex: "CSCI300")
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
// AVL Tree implementation (keyed by courseNumber)
// ==============================
//
// Stores Course objects in a self-balancing AVL tree using courseNumber
// as the sort key.
// Automatically maintains tree balance to ensure efficient insertion
// and searching.
// In-order traversal prints courses in alphanumeric order.
// Each AVL node stores a course, child pointers, and its height.
class CourseBST {
    struct Node {
        Course course;
        Node* left;
        Node* right;
        int height;

        // New nodes begin as leaf nodes with a height of 1.
        explicit Node(const Course& c)
            : course(c),
            left(nullptr),
            right(nullptr),
            height(1) {}
    };

    // Root pointer for the AVL tree (nullptr when tree is empty)
    Node* root = nullptr;

    // Returns the height of a node (0 if the node is null).
    int getHeight(Node* node) const {
        return node == nullptr ? 0 : node->height;
    }

    // Calculates the balance factor of a node.
    // Positive values indicate a taller left subtree.
    // Negative values indicate a taller right subtree.
    int getBalanceFactor(Node* node) const {
        if (node == nullptr) {
            return 0;
        }

        return getHeight(node->left) - getHeight(node->right);
    }

    // Recursively verifies that every node satisfies AVL balance requirements.
    bool isBalanced(Node* node) const {
        if (node == nullptr) {
            return true;
        }

        int balance = getBalanceFactor(node);

        // Each node must have a balance factor between -1 and 1.
        if (balance < -1 || balance > 1) {
            return false;
        }

        // Check both subtrees.
        return isBalanced(node->left) && isBalanced(node->right);
    }

    // Updates a node's height based on its children.
    void updateHeight(Node* node) {
        if (node != nullptr) {
            node->height = 1 + max(
                getHeight(node->left),
                getHeight(node->right)
            );
        }
    }

    // Performs a right rotation to balance a left-heavy subtree.
    Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* subtree = x->right;

        // Perform rotation.
        x->right = y;
        y->left = subtree;

        // Update heights after rotation.
        updateHeight(y);
        updateHeight(x);

        // Return the new subtree root.
        return x;
    }

    // Performs a left rotation to balance a right-heavy subtree.
    Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* subtree = y->left;

        // Perform rotation.
        y->left = x;
        x->right = subtree;

        // Update heights after rotation.
        updateHeight(x);
        updateHeight(y);

        // Return the new subtree root.
        return y;
    }

    // Recursively frees memory in post-order (children before parent).
    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    // Recursively inserts a course into the AVL tree.
    // Automatically restores balance after insertion.
    Node* insert(Node* node, const Course& course) {

        // Create a new node if the current subtree is empty.
        if (node == nullptr) {
            return new Node(course);
        }

        // Insert into the appropriate subtree.
        if (course.courseNumber < node->course.courseNumber) {
            node->left = insert(node->left, course);
        }
        else if (course.courseNumber > node->course.courseNumber) {
            node->right = insert(node->right, course);
        }
        else {
            // Update the existing course if its number already exists.
            node->course = course;
            return node;
        }

        // Update the height after insertion.
        updateHeight(node);

        // Calculate the balance factor.
        int balance = getBalanceFactor(node);

        // Case 1: Left-Left imbalance.
        if (balance > 1 &&
            course.courseNumber < node->left->course.courseNumber) {
            return rotateRight(node);
        }

        // Case 2: Right-Right imbalance.
        if (balance < -1 &&
            course.courseNumber > node->right->course.courseNumber) {
            return rotateLeft(node);
        }

        // Case 3: Left-Right imbalance.
        if (balance > 1 &&
            course.courseNumber > node->left->course.courseNumber) {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }

        // Case 4: Right-Left imbalance.
        if (balance < -1 &&
            course.courseNumber < node->right->course.courseNumber) {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        // Return the unchanged root if the subtree is balanced.
        return node;
    }

    // Recursively searches the self-balancing AVL tree by courseNumber.
    // Uses the course number to determine which subtree to explore.
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

    // Clears the tree contents (used when validation fails).
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

    // Displays the root node's height, balance factor,
    // and whether the entire AVL tree is balanced.
    void PrintAVLDiagnostics() const {
        if (root == nullptr) {
            cout << "AVL tree is empty." << endl;
            return;
        }

        cout << "\nAVL Tree Diagnostics:" << endl;
        cout << "Root Course: " << root->course.courseNumber << endl;
        cout << "Root Height: " << getHeight(root) << endl;
        cout << "Root Balance Factor: " << getBalanceFactor(root) << endl;

        // Verify that every node satisfies AVL balance requirements.
        if (isBalanced(root)) {
            cout << "AVL Balance Check: PASSED" << endl;
        }
        else {
            cout << "AVL Balance Check: FAILED" << endl;
        }
    }
};

// ==============================
// Directed Graph for Course Prerequisites
// ==============================

// Stores course prerequisite relationships using an adjacency list.
// Each course maps to the courses it requires.
class CourseGraph {
private:
    unordered_map<string, vector<string>> adjacencyList;

    // Recursively explores prerequisite relationships using DFS.
    // Tracks visited courses to avoid processing the same course twice.
    void dfsPrerequisites(const string& courseNumber,
                          unordered_set<string>& visited,
                          vector<string>& result) const {

        auto it = adjacencyList.find(courseNumber);

        // Stop if the course does not exist in the graph.
        if (it == adjacencyList.end()) {
            return;
        }

        // Explore each direct prerequisite.
        for (const string& prerequisite : it->second) {

            // Only process prerequisites that have not been visited.
            if (visited.insert(prerequisite).second) {

                // Explore indirect prerequisites first.
                dfsPrerequisites(prerequisite, visited, result);

                // Add the prerequisite after exploring its dependencies.
                result.push_back(prerequisite);
            }
        }
    }

    // Uses DFS to detect circular prerequisite relationships.
    // visiting tracks courses in the current recursion path.
    // visited tracks courses that have been fully explored.
    bool dfsDetectCycle(const string& courseNumber,
                        unordered_set<string>& visiting,
                        unordered_set<string>& visited) const {

        // A course already in the current path indicates a cycle.
        if (visiting.count(courseNumber)) {
            return true;
        }

        // Skip courses that have already been fully explored.
        if (visited.count(courseNumber)) {
            return false;
        }

        // Mark this course as part of the current DFS path.
        visiting.insert(courseNumber);

        auto it = adjacencyList.find(courseNumber);

        if (it != adjacencyList.end()) {
            for (const string& prerequisite : it->second) {
                if (dfsDetectCycle(prerequisite, visiting, visited)) {
                    return true;
                }
            }
        }

        // Remove the course from the current path.
        visiting.erase(courseNumber);

        // Mark the course as fully explored.
        visited.insert(courseNumber);

        return false;
    }

    // Uses DFS to build a valid course sequence.
    // Adds each course after all its prerequisites are processed.
    void dfsTopologicalSort(const string& courseNumber,
                            unordered_set<string>& visited,
                            vector<string>& courseSequence) const {

        // Skip courses that have already been processed.
        if (!visited.insert(courseNumber).second) {
            return;
        }

        // Explore all prerequisites before adding the course.
        auto it = adjacencyList.find(courseNumber);

        if (it != adjacencyList.end()) {
            for (const string& prerequisite : it->second) {
                dfsTopologicalSort(prerequisite, visited, courseSequence);
            }
        }

        // Add the course after its prerequisites.
        courseSequence.push_back(courseNumber);
    }

public:
    // Adds a course to the graph if it does not already exist.
    void AddCourse(const string& courseNumber) {
        adjacencyList.try_emplace(courseNumber);
    }

    // Removes all courses and prerequisite relationships.
    // Used when reloading course data.
    void Clear() {
        adjacencyList.clear();
    }

    // Adds a directed relationship from a course to its prerequisite.
    void AddPrerequisite(const string& courseNumber,
                         const string& prerequisite) {
        AddCourse(courseNumber);
        AddCourse(prerequisite);

        adjacencyList[courseNumber].push_back(prerequisite);
    }

    // Returns the direct prerequisites for a specified course.
    // Returns an empty list if the course is not found.
    vector<string> GetPrerequisites(const string& courseNumber) const {
        auto it = adjacencyList.find(courseNumber);

        if (it == adjacencyList.end()) {
            return {};
        }

        return it->second;
    }

    // Checks whether a course exists in the graph.
    bool HasCourse(const string& courseNumber) const {
        return adjacencyList.find(courseNumber) != adjacencyList.end();
    }

    // Returns all direct and indirect prerequisites for a course.
    // Uses DFS to place foundational prerequisites first.
    vector<string> GetCompletePrerequisiteChain(
        const string& courseNumber) const {

        vector<string> result;
        unordered_set<string> visited;

        // Return an empty list if the course does not exist.
        if (!HasCourse(courseNumber)) {
            return result;
        }

        // Mark the requested course as visited.
        visited.insert(courseNumber);

        // Explore all prerequisite relationships.
        dfsPrerequisites(courseNumber, visited, result);

        return result;
    }

    // Checks the entire graph for circular prerequisite relationships.
    // Returns true if a cycle exists and false otherwise.
    bool HasCycle() const {
        unordered_set<string> visiting;
        unordered_set<string> visited;

        // Check every course, including disconnected course groups.
        for (const auto& entry : adjacencyList) {
            const string& courseNumber = entry.first;

            if (dfsDetectCycle(courseNumber, visiting, visited)) {
                return true;
            }
        }

        return false;
    }

    // Generates a valid course sequence using topological sorting.
    // Ensures all prerequisites appear before their dependent courses.
    vector<string> GetTopologicalOrder() const {
        vector<string> courseSequence;
        unordered_set<string> visited;

        // Reject graphs containing circular prerequisites.
        if (HasCycle()) {
            return courseSequence;
        }

        // Process every course in the graph.
        for (const auto& entry : adjacencyList) {
            dfsTopologicalSort(entry.first, visited, courseSequence);
        }

        return courseSequence;
    }
};


// ==============================
// Loading + validation
// ==============================
//
// Loads and validates course data before making it available to the program.
// Validation rules:
//  1) Each non-empty line must contain a course number and course name.
//  2) Required course fields cannot be blank.
//  3) Every prerequisite must reference a course in the input file.
//  4) The prerequisite graph must not contain circular dependencies.
// Invalid data prevents the advising program from starting.
static bool LoadCoursesWithValidation(const string& filePath,
                                      CourseBST& tree,
                                      CourseGraph& graph) {
    // Attempt to open the input file
    ifstream infile(filePath);
    if (!infile.is_open()) {
        cout << "Error: Unable to open file: " << filePath << endl;
        return false;
    }

    // Temporarily stores courses for validation before loading the AVL tree and graph.
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

    // Clear existing data before loading validated courses.
    tree.Clear();
    graph.Clear();

    // Populate the AVL tree and directed prerequisite graph.
    for (const Course& c : tempCourses) {
        tree.Insert(c);
        graph.AddCourse(c.courseNumber);

        // Add each prerequisite relationship to the graph.
        for (const string& prerequisite : c.prerequisites) {
            graph.AddPrerequisite(c.courseNumber, prerequisite);
        }
    }

    // Check for circular prerequisite relationships.
    if (graph.HasCycle()) {
        cout << "Error: Circular prerequisite dependency detected." << endl;
        cout << "Course data cannot be loaded because the "
             << "prerequisite graph contains a cycle." << endl;

        // Clear invalid data from both structures.
        tree.Clear();
        graph.Clear();

        return false;
    }

    cout << "Courses loaded successfully (" << tempCourses.size()
         << " courses)." << endl;
    return true;
}


// ==============================
// Printing one course (Option 2)
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
    cout << "\nMenu:" << endl;
    cout << "  1. Print All Courses (Sorted)" << endl;
    cout << "  2. Search for a Course" << endl;
    cout << "  3. View Complete Prerequisite Chain" << endl;
    cout << "  4. Generate Valid Course Sequence" << endl;
    cout << "  5. Exit" << endl;
    cout << "Enter choice: ";
}


// ==============================
// Program Entry Point
// ==============================
#ifndef AVL_ROTATION_TEST
int main() {

    // Hardcoded input file path (per assignment instructions / user environment)
    // Note: Backslashes must be escaped in C++ string literals.
    string filePath = "CS 300 ABCU_Advising_Program_Input.csv";

    // Self-balancing AVL tree that stores all loaded courses.
    CourseBST coursesTree;

    // Directed graph that stores course prerequisite relationships.
    CourseGraph coursesGraph;

    // Automatically load and validate course data at startup.
    bool dataLoaded =
        LoadCoursesWithValidation(filePath, coursesTree, coursesGraph);

    // Stop the program if the course data cannot be loaded.
    if (!dataLoaded) {
        cout << "Error: The advising program cannot continue "
            << "without valid course data." << endl;
        return 1;
    }

    int choice = 0;

    // Continue displaying the menu until the user selects Exit (5).
    while (choice != 5) {
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
            // Option 1: Display all courses in sorted order using the AVL tree.
            cout << "\nCourse List (Sorted):" << endl;
            coursesTree.PrintInOrder();

            break;
        }

        case 2: {
            // Option 2: Search for a course using the AVL tree.
            cout << "Enter course number: ";
            string courseNumber;
            cin >> courseNumber;

            // Display the selected course and its direct prerequisites.
            PrintCourseAndPrereqs(coursesTree, courseNumber);

            break;
        }

        case 3: {
            // Option 3: Display all direct and indirect prerequisites.
            cout << "Enter course number: ";
            string courseNumber;
            cin >> courseNumber;

            // Verify that the requested course exists.
            if (!coursesGraph.HasCourse(courseNumber)) {
                cout << "Course not found: " << courseNumber << endl;
                break;
            }

            // Retrieve the complete prerequisite chain using DFS.
            vector<string> prerequisites =
                coursesGraph.GetCompletePrerequisiteChain(courseNumber);

            cout << "\nComplete Prerequisite Chain for "
                 << courseNumber << ":" << endl;

            if (prerequisites.empty()) {
                cout << "No prerequisites required." << endl;
            }
            else {
                for (const string& prerequisite : prerequisites) {
                    cout << "  " << prerequisite << endl;
                }
            }

            break;
        }

        case 4: {
            // Option 4: Generate a valid course sequence using topological sorting.
            vector<string> courseSequence =
                coursesGraph.GetTopologicalOrder();

            cout << "\nValid Course Sequence:" << endl;

            // Display courses in prerequisite-first order.
            if (courseSequence.empty()) {
                cout << "Unable to generate a course sequence." << endl;
            }
            else {
                for (const string& courseNumber : courseSequence) {
                    const Course* course = coursesTree.Search(courseNumber);

                    if (course != nullptr) {
                        cout << "  " << course->courseNumber
                             << ", " << course->courseName << endl;
                    }
                }
            }

            break;
        }

        case 5:
            // Option 5: Exit the advising program.
            cout << "Thank you for using the course planner! Goodbye." << endl;
            break;

        default:
            // Any other number is not a valid menu option
            cout << "Invalid option. Try again." << endl;
            break;
        }
    }

    return 0;
}

#endif // AVL_ROTATION_TEST