#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <windows.h>

#define BMP 0x4D42
using namespace std;

class SimpleBmp {
public:
    SimpleBmp() {}
    ~SimpleBmp() {};

    int getWidth() const { return infoHeader.biWidth; }
    int getHeight() const { return abs(infoHeader.biHeight); }

    bool load(const string& filename);
    void printToConsole(char blackChar = '.', char whiteChar = '#');
    void drawLine(int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b);
    bool save(const string& filename);

private:
    BITMAPFILEHEADER fileHeader{};
    BITMAPINFOHEADER infoHeader{};
    vector<unsigned char> data;
    int rowSize = 0;

    bool checkFormat() const;
    void plotPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b);
};

bool SimpleBmp::load(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error: File didn't opening!" << filename << '\n';
        return false;
    }

    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(fileHeader));
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(infoHeader));

    if (!checkFormat()) {
        cerr << "The BMP format is not supported - you need 24 or 32 bits without compression!\n";
        return false;
    }
    
    rowSize = ((infoHeader.biBitCount * infoHeader.biWidth + 31) / 32) * 4;
    int dataSize = rowSize * abs(infoHeader.biHeight);

    data.resize(dataSize);

    file.seekg(fileHeader.bfOffBits, ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), dataSize);

    file.close();
    return true;
}
bool SimpleBmp::checkFormat() const{
    if (fileHeader.bfType != BMP) {
        cerr << "Error: the file is not a BMP!\n";
        return false;
    }
    if (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32) {
        cerr << "Error: Only 24 and 32 bits per pixel are supported!\n";
        return false;
    }
    if (infoHeader.biCompression != 0) {
        cerr << "Error: Only uncompressed BMP is supported!\n";
        return false;
    }
    return true;
}
void SimpleBmp::printToConsole(char blackChar, char whiteChar) {
    for (int y = 0; y < infoHeader.biHeight; ++y) {
        int row = (infoHeader.biHeight > 0) ? (infoHeader.biHeight - 1 - y) : y;
        for (int x = 0; x < infoHeader.biWidth; ++x) {
            int bytesPerPixel = infoHeader.biBitCount / 8;
            int idx = row * rowSize + x * bytesPerPixel;
            unsigned char b = data[idx];
            unsigned char g = data[idx + 1];
            unsigned char r = data[idx + 2];

            char outputChar;
            if (r == 0 && g == 0 && b == 0) {
                outputChar = blackChar;
            }
            else if (r == 255 && g == 255 && b == 255) {
                outputChar = whiteChar;
            }
            else {
                outputChar = '?';
            }
            cout << outputChar << outputChar; // Двойной вывод сделан для пропорциональной отрисовки рисунока в консоли.
        }
        cout << '\n';
    }
}
void SimpleBmp::plotPixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x < 0 || x >= infoHeader.biWidth || y < 0 || y >= abs(infoHeader.biHeight)) {
        return;
    }
    int bytesPerPixel = infoHeader.biBitCount / 8;
    int row = (infoHeader.biHeight > 0) ? (abs(infoHeader.biHeight) - 1 - y) : y;
    int idx = row * rowSize + x * bytesPerPixel;

    data[idx] = b;
    data[idx + 1] = g;
    data[idx + 2] = r;
    if (bytesPerPixel == 4) {
        data[idx + 3] = 255;
    }
}
void SimpleBmp::drawLine(int x1, int y1, int x2, int y2, unsigned char r, unsigned char b, unsigned char g) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        plotPixel(x1, y1, r, g, b);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}
bool SimpleBmp::save(const string& filename) {
    FILE* f;
    errno_t err = fopen_s(&f, filename.c_str(), "wb");
    if (err != 0 || !f) {
        std::cerr << "Error opening the file for recording: " << filename << '\n';
        return false;
    }

    int bytesPerPixel = infoHeader.biBitCount / 8;
    int rowSize = (infoHeader.biWidth * bytesPerPixel + 3) & (~3);
    
    BITMAPFILEHEADER fileHeader;
    fileHeader.bfType = BMP;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + infoHeader.biSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfSize = fileHeader.bfOffBits + rowSize * infoHeader.biHeight;

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, f);
    fwrite(&infoHeader, infoHeader.biSize, 1, f);

    if (infoHeader.biHeight > 0) {
        for (int y = infoHeader.biHeight - 1; y >= 0; --y) {
            fwrite(data.data() + y * rowSize, 1, rowSize, f);
        }
    }
    else {
        fwrite(data.data(), 1, rowSize * infoHeader.biHeight, f);
    }
    
    fclose(f);
    return true;
}
int main() {
    string inputFilename;
    cout << "Enter input BMP file name: ";
    getline(cin, inputFilename);

    SimpleBmp bmp;
    if (!bmp.load(inputFilename)) {
        cerr << "Failed to upload file: " << inputFilename << endl;
        return 1;
    }

    cout << "The file has been successfully uploaded and verified.\n";
    cout << "The original image:\n";
    bmp.printToConsole('.', '#');

    int width = bmp.getWidth();
    int height = abs(bmp.getHeight());

    bmp.drawLine(0, 0, width - 1, height - 1, 255, 0, 0);      
    bmp.drawLine(0, height - 1, width - 1, 0, 255, 0, 0);

    cout << "\nThe image after drawing the cross:\n";
    bmp.printToConsole('.', '#');

    string outputFilename;
    cout << "Enter the file name to save the modified image: ";
    getline(std::cin, outputFilename);

    if (!bmp.save(outputFilename)) {
        cerr << "Error saving the file!\n";
        return 1;
    }
    cout << "The modified image is saved as " << outputFilename << endl;
    return 0;
}