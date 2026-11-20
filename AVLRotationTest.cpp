// Reuse the AVL tree implementation from the advising program.
// Exclude the original main() function during rotation testing.
#define AVL_ROTATION_TEST
#include "ProjectTwo.cpp"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

// Represents a course used in AVL rotation testing.
struct TestCourse {
    string courseNumber;
    string courseName;
};

// Represents one test case and its insertion order.
struct RotationTest {
    string testName;
    vector<string> insertionOrder;
    string expectedRoot;
};

int main() {

    // Test cases designed to trigger each AVL rotation.
    vector<RotationTest> tests = {
        {"Left-Left (LL)", {"CSCI300", "CSCI200", "CSCI100"}, "CSCI200"},
        {"Right-Right (RR)", {"CSCI100", "CSCI200", "CSCI300"}, "CSCI200"},
        {"Left-Right (LR)", {"CSCI300", "CSCI100", "CSCI200"}, "CSCI200"},
        {"Right-Left (RL)", {"CSCI100", "CSCI300", "CSCI200"}, "CSCI200"}
    };

    cout << "AVL Rotation Test Cases" << endl;

    for (const RotationTest& test : tests) {
        cout << "\nTest: " << test.testName << endl;

        // Create a fresh AVL tree for each rotation test.
        CourseBST testTree;

        // Insert courses in the order specified by the test.
        for (const string& courseNumber : test.insertionOrder) {
            Course course;
            course.courseNumber = courseNumber;
            course.courseName = "Test Course";

            testTree.Insert(course);
        }

        // Display the resulting tree's root, height, and balance.
        testTree.PrintAVLDiagnostics();

        cout << "Insertion Order: ";
        for (const string& courseNumber : test.insertionOrder) {
            cout << courseNumber << " ";
        }

        cout << "\nExpected Root: " << test.expectedRoot << endl;
    }

    return 0;
}