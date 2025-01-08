#include "Bitmap.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#define _USE_MATH_DEFINES
#include <math.h>

struct Params
{
    Bitmap* in;
    uint32_t startHeight;
    uint32_t endHeight;
    uint32_t startWidth;
    uint32_t endWidth;
};

void Blur(int radius, Params* params)
{
    float rs = ceil(radius * 3);
    float sigma = radius / 2.0;
    float twoPiSigmaSquare = 2.0 * M_PI * sigma * sigma;
    for (auto i = params->startHeight; i < params->endHeight; ++i)
    {
        for (auto j = params->startWidth; j < params->endWidth; ++j)
        {
            double r = 0, g = 0, b = 0;
            double weightSum = 0;

            for (int iy = i - rs; iy < i + rs + 1; ++iy)
            {
                for (int ix = j - rs; ix < j + rs + 1; ++ix)
                {
                    auto x = min(static_cast<int>(params->endWidth) - 1, max(0, ix));
                    auto y = min(static_cast<int>(params->endHeight) - 1, max(0, iy));

                    float distanceSquare = ((ix - j) * (ix - j)) + ((iy - i) * (iy - i));
                    float weight = exp(-distanceSquare / (2.0 * sigma * sigma)) / twoPiSigmaSquare;

                    rgb32* pixel = params->in->GetPixel(x, y);

                    r += pixel->r * weight;
                    g += pixel->g * weight;
                    b += pixel->b * weight;
                    weightSum += weight;
                }
            }

            rgb32* pixel = params->in->GetPixel(j, i);
            pixel->r = std::round(r / weightSum);
            pixel->g = std::round(g / weightSum);
            pixel->b = std::round(b / weightSum);
        }
    }
}

DWORD WINAPI StartThreads(CONST LPVOID lpParam)
{
    struct Params* params = (struct Params*)lpParam;
    Blur(10, params);
    ExitThread(0);
}

void StartThreads(Bitmap* in, int threadsCount, int coreCount)
{
    int partHeight = in->GetHeight() / threadsCount;
    int heightRemaining = in->GetHeight() % threadsCount;

    Params* arrayParams = new Params[threadsCount];
    for (int i = 0; i < threadsCount; i++)
    {
        Params params;
        params.in = in;
        params.startWidth = 0;
        params.endWidth = in->GetWidth();
        params.startHeight = partHeight * i;
        params.endHeight = partHeight * (i + 1) + ((i == threadsCount - 1) ? heightRemaining : 0);
        arrayParams[i] = params;
    }

    HANDLE* handles = new HANDLE[threadsCount];
    for (int i = 0; i < threadsCount; i++)
    {
        handles[i] = CreateThread(NULL, i, &StartThreads, &arrayParams[i], CREATE_SUSPENDED, NULL);
        SetThreadAffinityMask(handles[i], (1 << coreCount) - 1);
    }

    for (int i = 0; i < threadsCount; i++)
    {
        ResumeThread(handles[i]);
    }


    WaitForMultipleObjects(threadsCount, handles, true, INFINITE);

    delete[] arrayParams;
    delete[] handles;
}

int main(int argc, const char** argv)
{
    double start = clock();

    Bitmap bmp{ "formula.bmp" };
    StartThreads(&bmp, 16, 4);
    bmp.Save("bluredImage.bmp");

    std::cout << (double)(clock() - start) / CLOCKS_PER_SEC << " s." << std::endl;

    return 0;
}