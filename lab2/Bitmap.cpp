#include "Bitmap.h"
#include <iostream>
#include <fstream>

Bitmap::Bitmap(const char* path)
    : m_bmpInfo()
    , m_pixels(nullptr)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (file)
    {
        file.read(reinterpret_cast<char*>(&m_bmpInfo.bfh), sizeof(m_bmpInfo.bfh));
        file.read(reinterpret_cast<char*>(&m_bmpInfo.bih), sizeof(m_bmpInfo.bih));
        file.seekg(m_bmpInfo.bfh.bfOffBits, std::ios::beg);

        m_pixels = new uint8_t[m_bmpInfo.bfh.bfSize - m_bmpInfo.bfh.bfOffBits];
        file.read(reinterpret_cast<char*>(&m_pixels[0]), m_bmpInfo.bfh.bfSize - m_bmpInfo.bfh.bfOffBits);


        uint8_t* temp = new uint8_t[m_bmpInfo.bih.biWidth * m_bmpInfo.bih.biHeight * sizeof(rgb32)];

        uint8_t* in = m_pixels;
        rgb32* out = reinterpret_cast<rgb32*>(temp);
        int padding = m_bmpInfo.bih.biBitCount == 24 ? ((m_bmpInfo.bih.biSizeImage - m_bmpInfo.bih.biWidth * m_bmpInfo.bih.biHeight * 3) / m_bmpInfo.bih.biHeight) : 0;

        auto alpha = m_bmpInfo.bih.biBitCount == 32 ? *(in++) : 0xFF;

        for (int i = 0; i < m_bmpInfo.bih.biHeight; ++i, in += padding)
        {
            for (int j = 0; j < m_bmpInfo.bih.biWidth; ++j)
            {

                out->b = *(in++);
                out->g = *(in++);
                out->r = *(in++);
                out->a = alpha;
                ++out;
            }
        }

        delete[] m_pixels;
        m_pixels = temp;
    }
}

Bitmap::~Bitmap()
{
    delete[] m_pixels;
}

void Bitmap::Save(const char* path, uint16_t bit_count)
{
    std::ofstream file(path, std::ios::out | std::ios::binary);

    if (file)
    {
        m_bmpInfo.bih.biBitCount = bit_count;
        uint32_t size = ((m_bmpInfo.bih.biWidth * m_bmpInfo.bih.biBitCount + 31) / 32) * 4 * m_bmpInfo.bih.biHeight;
        m_bmpInfo.bfh.bfSize = m_bmpInfo.bfh.bfOffBits + size;

        file.write(reinterpret_cast<char*>(&m_bmpInfo.bfh), sizeof(m_bmpInfo.bfh));
        file.write(reinterpret_cast<char*>(&m_bmpInfo.bih), sizeof(m_bmpInfo.bih));
        file.seekp(m_bmpInfo.bfh.bfOffBits, std::ios::beg);

        uint8_t* out = NULL;
        rgb32* in = reinterpret_cast<rgb32*>(m_pixels);
        uint8_t* temp = out = new uint8_t[m_bmpInfo.bih.biWidth * m_bmpInfo.bih.biHeight * sizeof(rgb32)];
        int padding = m_bmpInfo.bih.biBitCount == 24 ? ((m_bmpInfo.bih.biSizeImage - m_bmpInfo.bih.biWidth * m_bmpInfo.bih.biHeight * 3) / m_bmpInfo.bih.biHeight) : 0;

        for (int i = 0; i < m_bmpInfo.bih.biHeight; ++i, out += padding)
        {
            for (int j = 0; j < m_bmpInfo.bih.biWidth; ++j)
            {
                *(out++) = in->b;
                *(out++) = in->g;
                *(out++) = in->r;

                if (m_bmpInfo.bih.biBitCount == 32)
                {
                    *(out++) = in->a;
                }
                ++in;
            }
        }

        file.write(reinterpret_cast<char*>(&temp[0]), size);
        delete[] temp;
    }
}

rgb32* Bitmap::GetPixel(uint32_t x, uint32_t y) const
{
    rgb32* temp = reinterpret_cast<rgb32*>(m_pixels);
    return &temp[(m_bmpInfo.bih.biHeight - 1 - y) * m_bmpInfo.bih.biWidth + x];
}

void Bitmap::SetPixel(rgb32* pixel, uint32_t x, uint32_t y)
{
    rgb32* temp = reinterpret_cast<rgb32*>(m_pixels);
    memcpy(&temp[(m_bmpInfo.bih.biHeight - 1 - y) * m_bmpInfo.bih.biWidth + x], pixel, sizeof(rgb32));
};

uint32_t Bitmap::GetWidth() const
{
    return m_bmpInfo.bih.biWidth;
}

uint32_t Bitmap::GetHeight() const
{
    return m_bmpInfo.bih.biHeight;
}

uint16_t Bitmap::BitCount() const
{
    return m_bmpInfo.bih.biBitCount;
}