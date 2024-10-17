#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <tuple>  // ��� std::make_tuple
#include <cmath>  // ��� std::sqrt()

// ��������� ��� �������� BMP-����������
#pragma pack(push, 1)
struct BMPHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct DIBHeader {
    uint32_t biSize;
    int32_t biWidth;
    int32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t biXPelsPerMeter;
    int32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

// ��������� ��� �������� �������� RGB
struct RGB {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

// ������� ��� ���������� ������� �������� (��������, 3x3 ������� ������)
void applyBlurFilter(std::vector<std::vector<RGB>>& pixels, int startX, int startY, int endX, int endY, int width, int height) {
    int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    int kernelSum = 16;  // ����� ����

    std::vector<std::vector<RGB>> tempPixels = pixels;  // ��� ���������� ���������

    for (int y = startY; y < endY && y < height; ++y) {
        for (int x = startX; x < endX && x < width; ++x) {
            if (x > 0 && x < width - 1 && y > 0 && y < height - 1) {
                int redSum = 0, greenSum = 0, blueSum = 0;
                for (int ky = -1; ky <= 1; ++ky) {
                    for (int kx = -1; kx <= 1; ++kx) {
                        int px = x + kx;
                        int py = y + ky;

                        redSum += pixels[py][px].red * kernel[ky + 1][kx + 1];
                        greenSum += pixels[py][px].green * kernel[ky + 1][kx + 1];
                        blueSum += pixels[py][px].blue * kernel[ky + 1][kx + 1];
                    }
                }
                tempPixels[y][x].red = redSum / kernelSum;
                tempPixels[y][x].green = greenSum / kernelSum;
                tempPixels[y][x].blue = blueSum / kernelSum;
            }
        }
    }

    // �������� ���������� ������� � ������������ ������
    for (int y = startY; y < endY && y < height; ++y) {
        for (int x = startX; x < endX && x < width; ++x) {
            pixels[y][x] = tempPixels[y][x];
        }
    }
}

// ������� ��� ��������� �������� � ������
DWORD WINAPI ThreadProc(LPVOID lpParam) {
    auto params = reinterpret_cast<std::tuple<std::vector<std::vector<RGB>>*, int, int, int, int, int, int>*>(lpParam);
    auto pixels = std::get<0>(*params);
    int startX = std::get<1>(*params);
    int startY = std::get<2>(*params);
    int endX = std::get<3>(*params);
    int endY = std::get<4>(*params);
    int width = std::get<5>(*params);
    int height = std::get<6>(*params);

    applyBlurFilter(*pixels, startX, startY, endX, endY, width, height);
    ExitThread(0);
}

// ������� ��� ������ BMP-�����
bool readBMP(const std::string& filename, BMPHeader& bmpHeader, DIBHeader& dibHeader, std::vector<std::vector<RGB>>& pixels) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    // ������ ����������
    file.read(reinterpret_cast<char*>(&bmpHeader), sizeof(BMPHeader));
    file.read(reinterpret_cast<char*>(&dibHeader), sizeof(DIBHeader));

    if (bmpHeader.bfType != 0x4D42) {
        std::cerr << "Error: Not a valid BMP file" << std::endl;
        return false;
    }

    // ������ ��������
    pixels.resize(dibHeader.biHeight, std::vector<RGB>(dibHeader.biWidth));
    file.seekg(bmpHeader.bfOffBits, file.beg);

    for (int y = 0; y < dibHeader.biHeight; ++y) {
        file.read(reinterpret_cast<char*>(pixels[y].data()), dibHeader.biWidth * sizeof(RGB));
    }

    file.close();
    return true;
}

// ������� ��� ������ BMP-�����
bool writeBMP(const std::string& filename, const BMPHeader& bmpHeader, const DIBHeader& dibHeader, const std::vector<std::vector<RGB>>& pixels) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return false;
    }

    // ������ ����������
    file.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(BMPHeader));
    file.write(reinterpret_cast<const char*>(&dibHeader), sizeof(DIBHeader));

    // ������ ��������
    for (int y = 0; y < dibHeader.biHeight; ++y) {
        file.write(reinterpret_cast<const char*>(pixels[y].data()), dibHeader.biWidth * sizeof(RGB));
    }

    file.close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <threads> <cores>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    int numThreads = std::stoi(argv[3]);
    int numCores = std::stoi(argv[4]);

    BMPHeader bmpHeader;
    DIBHeader dibHeader;
    std::vector<std::vector<RGB>> pixels;

    // ������ �������� BMP �����
    if (!readBMP(inputFile, bmpHeader, dibHeader, pixels)) {
        return 1;
    }

    int width = dibHeader.biWidth;
    int height = dibHeader.biHeight;

    // ��������� ����� ����������� (SetThreadAffinityMask)
    DWORD_PTR affinityMask = (1 << numCores) - 1;

    // ����� ������ ����������
    auto start = std::chrono::high_resolution_clock::now();

    // �������� ������� ��� ���������
    HANDLE* threads = new HANDLE[numThreads];
    std::tuple<std::vector<std::vector<RGB>>*, int, int, int, int, int, int>* params = new std::tuple<std::vector<std::vector<RGB>>*, int, int, int, int, int, int>[numThreads];

    // ��������� �� ��������
    int sqrtThreads = static_cast<int>(std::sqrt(numThreads));
    int squareWidth = width / sqrtThreads;
    int squareHeight = height / sqrtThreads;

    int threadIndex = 0;
    for (int i = 0; i < sqrtThreads; ++i) {
        for (int j = 0; j < sqrtThreads; ++j) {
            int startX = j * squareWidth;
            int startY = i * squareHeight;
            int endX = (j + 1) * squareWidth;
            int endY = (i + 1) * squareHeight;

            params[threadIndex] = std::make_tuple(&pixels, startX, startY, endX, endY, width, height);

            threads[threadIndex] = CreateThread(NULL, 0, ThreadProc, &params[threadIndex], 0, NULL);
            SetThreadAffinityMask(threads[threadIndex], affinityMask);

            ++threadIndex;
        }
    }

    // �������� ���������� ���� �������
    WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

    // ����� ��������� ����������
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    // ������ ��������� BMP �����
    if (!writeBMP(outputFile, bmpHeader, dibHeader, pixels)) {
        return 1;
    }

    // ������������ ��������
    for (int i = 0; i < numThreads; ++i) {
        CloseHandle(threads[i]);
    }
    delete[] threads;
    delete[] params;

    // ����� ������� ����������
    std::cout << "Time taken: " << elapsed.count() << " ms" << std::endl;

    return 0;
}
