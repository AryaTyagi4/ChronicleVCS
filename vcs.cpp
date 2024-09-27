#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <unordered_map>
#include <vector>
#include <windows.h> // For Windows-specific directory functions
#include <sstream>
#include <unordered_set>

using namespace std;

void initRepository();
void addFileToVCS(const string& filename);
void commitChanges(const string& message);
void viewCommitHistory();
void revertToCommit(const string& commitID);
string generateCommitID(const string& fileContents);
vector<string> listTrackedFiles();

// Initialize the repository
void initRepository() {
    CreateDirectoryA(".vcs", NULL);
    ofstream file(".vcs/commit_log.txt");
    file << "Version Control Initialized.\n";
    file.close();
    cout << "Repository Initialized.\n";
}

// Add a file to version control
void addFileToVCS(const string& filename) {
    ifstream src(filename, ios::binary);
    if (!src) {
        cerr << "File not found: " << filename << "\n";
        return;
    }
    
    // Create the directory for tracked files if it doesn't exist
    CreateDirectoryA(".vcs/tracked_files", NULL);
    
    ofstream dest(".vcs/tracked_files/" + filename, ios::binary);
    dest << src.rdbuf();
    cout << "File " << filename << " added to version control.\n";
}

// Generate a unique commit ID using hash
string generateCommitID(const string& fileContents) {
    hash<string> hashFunc;
    return to_string(hashFunc(fileContents)) + "_" + to_string(time(nullptr));
}

// List tracked files in the directory
vector<string> listTrackedFiles() {
    vector<string> files;
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind;

    // Look for files in the tracked files directory
    hFind = FindFirstFileA(".vcs/tracked_files/*", &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return files; // Return empty if no files found
    } 
    do {
        string filename = findFileData.cFileName;
        // Ignore . and .. files
        if (filename != "." && filename != "..") {
            files.push_back(filename);
        }
    } while (FindNextFileA(hFind, &findFileData) != 0);
    FindClose(hFind);
    return files;
}

// Commit changes with a message
void commitChanges(const string& message) {
    auto trackedFiles = listTrackedFiles();

    if (trackedFiles.empty()) {
        cerr << "No tracked files found.\n";
        return;
    }

    // Display tracked files
    cout << "Tracked files:\n";
    for (size_t i = 0; i < trackedFiles.size(); ++i) {
        cout << i + 1 << ": " << trackedFiles[i] << "\n";
    }

    cout << "Select files to commit (e.g., 1 3 for first and third files, A for all, or 0 to cancel): ";
    string input;
    getline(cin >> ws, input); // Use ws to ignore leading whitespace

    vector<int> selectedFiles;

    // Check if the user selected all files
    if (input == "A" || input == "a") {
        for (size_t i = 0; i < trackedFiles.size(); ++i) {
            selectedFiles.push_back(i); // Add all tracked files
        }
    } else {
        // Parse user input for specific file indices
        stringstream ss(input);
        int index;
        while (ss >> index) {
            if (index == 0) {
                cout << "Commit canceled.\n";
                return; // Cancel commit if user selects 0
            } else if (index > 0 && index <= trackedFiles.size()) {
                selectedFiles.push_back(index - 1); // Store the selected index (0-based)
            } else {
                cout << "Invalid selection: " << index << "\n";
            }
        }
    }

    // Using a set to track commit IDs for uniqueness
    unordered_set<string> committedFiles;

    ofstream commitLog(".vcs/commit_log.txt", ios::app);
    if (!commitLog.is_open()) {
        cerr << "Failed to open commit log.\n";
        return;
    }

    for (int i : selectedFiles) {
        const string& filename = trackedFiles[i];

        // Check if this file has already been committed in this operation
        if (committedFiles.find(filename) != committedFiles.end()) {
            cout << "File " << filename << " has already been committed. Skipping...\n";
            continue;
        }

        ifstream file(".vcs/tracked_files/" + filename);
        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        string commitID = generateCommitID(content);

        // Store the commit ID for this file
        committedFiles.insert(filename);

        commitLog << "Commit ID: " << commitID << "\n";
        commitLog << "Message: " << message << "\n";
        commitLog << "File: " << filename << "\n";
        commitLog << "Timestamp: " << time(0) << "\n\n";
        cout << "Changes committed with ID: " << commitID << " for file: " << filename << "\n";
    }
}

// View commit history
void viewCommitHistory() {
    ifstream logFile(".vcs/commit_log.txt");
    if (!logFile) {
        cerr << "No commit history available.\n";
        return;
    }
    string line;
    while (getline(logFile, line)) {
        cout << line << "\n";
    }
}

// Revert to a previous version using commit ID
void revertToCommit(const string& commitID) {
    ifstream logFile(".vcs/commit_log.txt");
    if (!logFile) {
        cerr << "No commit history available.\n";
        return;
    }

    string line;
    bool found = false;

    // Read through the commit log to find the commit ID
    while (getline(logFile, line)) {
        if (line.find("Commit ID: " + commitID) != string::npos) {
            found = true; // Found the commit
            // Read the subsequent lines to find file names
            while (getline(logFile, line) && !line.empty()) {
                if (line.find("File: ") != string::npos) {
                    string filename = line.substr(line.find(": ") + 2);
                    // Restore the file from the tracked files directory
                    ifstream trackedFile(".vcs/tracked_files/" + filename, ios::binary);
                    if (trackedFile) {
                        ofstream dest(filename, ios::binary);
                        dest << trackedFile.rdbuf(); // Copy content back to original file
                        cout << "Reverted " << filename << " to commit " << commitID << ".\n";
                    } else {
                        cerr << "File " << filename << " not found in tracked files.\n";
                    }
                }
            }
            break; // Exit the loop after finding the commit
        }
    }

    if (!found) {
        cerr << "Commit ID not found: " << commitID << "\n";
    }
}


void viewFileContents(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Could not open the file: " << filename << "\n";
        return;
    }

    cout << "Contents of " << filename << ":\n";
    string line;
    while (getline(file, line)) {
        cout << line << "\n";
    }
    file.close();
}
void removeFileFromVCS(const string& filename) {
    string filePath = ".vcs/tracked_files/" + filename;
    
    // Attempt to delete the file
    if (remove(filePath.c_str()) != 0) {
        cerr << "Error deleting file: " << filename << "\n";
    } else {
        cout << "File " << filename << " removed from version control.\n";
    }
}

void writeToFile(const string& filename, const string& content) {
    ofstream file(filename, ios::app); // Open file in append mode
    if (!file) {
        cerr << "Error opening file: " << filename << "\n";
        return;
    }
    file << content << "\n"; // Write content to the file
    file.close(); // Close the file
    cout << "Content written to " << filename << " successfully.\n";
}
// Main function to drive the VCS
int main() {
    cout << "Welcome to Basic VCS\n";
    string command, content, filename, message, commitID;

    while (true) {
        cout << "\nEnter command (init, add, commit, write, view, log, revert, remove, exit): ";
        cin >> command;

        if (command == "init") {
            initRepository();
        } else if (command == "add") {
            cout << "Enter filename to add: ";
            cin >> filename;
            addFileToVCS(filename);
        } else if (command == "commit") {
            cout << "Enter commit message: ";
            cin.ignore(); // Clear the newline from the input buffer
            getline(cin, message);
            commitChanges(message);
        } else if (command == "log") {
            viewCommitHistory();
        } else if (command == "revert") {
            cout << "Enter commit ID to revert to: ";
            cin >> commitID;
            revertToCommit(commitID);
        } else if (command == "remove") {
            cout << "Enter filename to remove from tracking: ";
            cin >> filename;
            removeFileFromVCS(filename);
        } else if (command == "view") {
            cout << "Enter filename to view contents: ";
            cin >> filename;
            viewFileContents(filename);
        } else if (command == "write") {
            cout << "Enter filename to write to: ";
            cin >> filename;
            cout << "Enter content to write: ";
            cin.ignore(); // Clear the newline from the input buffer
            getline(cin, content);
            writeToFile(filename, content);
        } else if (command == "exit") {
            break; // Exit the loop
        } else {
            cout << "Invalid command. Try again.\n"; // Prompt for valid command
        }
    }
    return 0;
}
