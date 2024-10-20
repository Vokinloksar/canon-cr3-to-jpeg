#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;

bool extractThumbnail(const char *inputFile, const char *outputFile) {
    // Open the input CR3 file
    ifstream inFile(inputFile, ios::binary);
    if (!inFile) {
        cerr << "Error opening input file." << endl;
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
                        cerr << "Error creating output file." << endl;
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
        cerr << "No JPEG thumbnail found." << endl;
        return false;
    }

    cout << "Thumbnail extracted as '" << outputFile << "'" << endl;
    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <input.CR3> <output.jpg>" << endl;
        return 1;
    }

    const char *inputFile = argv[1];
    const char *outputFile = argv[2];

    if (!extractThumbnail(inputFile, outputFile)) {
        return 1;
    }

    return 0;
}

