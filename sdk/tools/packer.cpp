#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <stdint.h>
#include <cstring>
#include <vector>
#include <iterator>

using std::vector;

vector<char> readToBuffer(FILE *fileDescriptor) {
    const unsigned N = 1024;

    fseek(fileDescriptor, 0, SEEK_END);
    auto endPos = ftell(fileDescriptor);
    rewind(fileDescriptor);
    vector<char> total(endPos);
    auto writeHead = std::begin(total);

    for (int c = 0; c < endPos; ++c) {
        char buffer[N];
        size_t read = fread((void *) &buffer[0], 1, N, fileDescriptor);
        if (read) {
            for (unsigned int c = 0; c < read; ++c) {
                *writeHead = (buffer[c]);
                writeHead = std::next(writeHead);
            }
        }
        if (read < N) {
            break;
        }
    }

    return total;
}

struct Entry {
public:
    Entry(std::string path, std::string key);

    vector<char> mContent;
    std::string id;
    uint32_t offset = 0;
};

Entry::Entry(std::string path, std::string key) : id(key) {
    FILE *in = fopen(path.c_str(), "rb");
    mContent = readToBuffer(in);
    fclose(in);
}

void writeHeader(FILE *file, uint16_t entries) {
    fwrite(&entries, 2, 1, file);
}

int main(int argc, char **argv) {
    int c = argc;

    vector<Entry> files;
    while (--c) {
        std::string filename = argv[c];
        std::cout << "adding " << filename << std::endl;
        files.emplace_back(filename, filename.substr(filename.find('/') + 1));
    }

    FILE *file = fopen("data.pfs", "wb");
    uint32_t accOffset = 0;
    uint16_t entries = files.size();
    writeHeader(file, entries);

    accOffset += 2;

    for (uint16_t e = 0; e < entries; ++e) {
        accOffset += 4 + 1 + files[e].id.length() + 1;
    }

    std::cout << "base offset " << accOffset << std::endl;

    for (uint16_t e = 0; e < entries; ++e) {
        files[e].offset = accOffset;
        std::cout << "Offset for " << files[e].id << " = " << accOffset << std::endl;
        accOffset += 4 + files[e].mContent.size();
    }

    for (uint16_t e = 0; e < entries; ++e) {
        std::cout << "entry " << files[e].id << " starting at " << ftell(file) << std::endl;
        uint32_t offset = files[e].offset;
        const char *name = files[e].id.c_str();
        uint8_t strLen = std::strlen(name);
        fwrite(&offset, 4, 1, file);
        fwrite(&strLen, 1, 1, file);
        fwrite(name, strLen + 1, 1, file);
    }

    for (uint16_t e = 0; e < entries; ++e) {
        uint32_t size = files[e].mContent.size();
        std::cout << "writing a file " << files[e].id << " with " << size << " bytes at " << ftell(file) << std::endl;
        fwrite(&size, 4, 1, file);

        for (const auto &c : files[e].mContent) {
            fwrite(&c, 1, 1, file);
        }
    }

    fclose(file);

    return 0;
}
