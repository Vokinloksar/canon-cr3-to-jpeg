#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

bool extractThumbnail(const char *inputFile, const char *outputFile) {
    // Open the input CR3 file
    ifstream inFile(inputFile, ios::binary);
    if (!inFile) {
        cerr << "Error opening input file: " << inputFile << endl;
        return false;
    }

    // Get the file size
    inFile.seekg(0, ios::end);
    size_t fileSize = inFile.tellg();
    inFile.seekg(0, ios::beg);

    // Read the entire file into a buffer
    char *buf = new char[fileSize];
    inFile.read(buf, fileSize);
    inFile.close();

    // Variables to find the JPEG thumbnail
    bool found = false;
    size_t n = 0;

    while (n < fileSize - 4 && !found) {
        if (memcmp(&buf[n], "mdat", 4) == 0) {
            if (memcmp(&buf[n + 12], "\xFF\xD8", 2) == 0) { // JPEG start marker
                size_t i = 0;
                while (!found && (n + 12 + i < fileSize - 1)) {
                    if (memcmp(&buf[n + 12 + i], "\xFF\xD9", 2) == 0) { // JPEG end marker
                        found = true;
                    }
                    i++;
                }
                if (found) {
                    // Write the thumbnail to the output file
                    ofstream outFile(outputFile, ios::binary);
                    if (!outFile) {
                        cerr << "Error creating output file: " << outputFile << endl;
                        delete[] buf;
                        return false;
                    }
                    // Write JPEG header and data
                    outFile.write(&buf[n + 12], i + 2); // +2 for the end marker
                    outFile.close();
                }
            } else {
                break;
            }
        } else {
            n++;
        }
    }

    // Clean up
    delete[] buf;

    if (!found) {
        cerr << "No JPEG thumbnail found in: " << inputFile << endl;
        return false;
    }

    cout << "Thumbnail extracted as '" << outputFile << "'" << endl;
    return true;
}

string getOutputFileName(const fs::path& inputFile) {
    return inputFile.stem().string() + ".jpg";  // Use stem() to get the filename without extension
}
bool copyExifData(const std::string& sourceFile, const std::string& destinationFile) {
    // Build the command string
    std::string command = "exiftool -overwrite_original -tagsFromFile \"" + sourceFile + "\" -all:all \"" + destinationFile + "\"";

    // Execute the command
    int result = system(command.c_str());

    // Check if the command was successful
    if (result != 0) {
        cerr << "Error executing command: " << command << endl;
        return false;
    }

    cout << "EXIF data copied from " << sourceFile << " to " << destinationFile << endl;
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_directory>" << endl;
        return 1;
    }

    fs::path inputDir(argv[1]);

    if (!fs::is_directory(inputDir)) {
        cerr << "Error: " << argv[1] << " is not a valid directory." << endl;
        return 1;
    }

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".CR3") {
            const auto& inputFile = entry.path();
            string outputFile = getOutputFileName(inputFile);
            outputFile = inputDir / outputFile; // Output in the same directory

            if (!extractThumbnail(inputFile.string().c_str(), outputFile.c_str())) {
                return 1;
            }
            if (!copyExifData(inputFile.string().c_str(), outputFile.c_str())) {
                return 1;
            }

        }
    }

    return 0;
}


