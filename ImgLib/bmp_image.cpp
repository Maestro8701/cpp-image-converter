#include "bmp_image.h"
#include "pack_defines.h"
#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    std::array<char, 2> signature = {'B', 'M'};
    unsigned int size = 0; // Расчетный размер
    unsigned int reserve = 0; // Заполненные нулями
    unsigned int step = 54; // Размер заголовка
} PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    unsigned int size_heading = 40; // 4 байта
    int width = 0; // 4 байта
    int height = 0; // 4 байта
    unsigned short planes = 1; // 2 байта
    unsigned short bits_per_pixel = 24; // 2 байта
    unsigned int compression_type = 0; // 4 байта
    unsigned int size_image = 0; // 4 байта
    int horizontal_resolution = 11811; // 4 байта
    int vertical_resolution = 11811; // 4 байта
    unsigned int colors_used = 0; // 4 байта
    unsigned int important_colors = 0x1000000; // 4 байта
} PACKED_STRUCT_END

static int GetBMPStride(int width) {
    return 4 * ((width * 3 + 3) / 4);
}

// Функция для сохранения изображения
bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    BitmapFileHeader bitmapfileheader;
    BitmapInfoHeader bitmapinfoheader;

    int stride = GetBMPStride(image.GetWidth());
    bitmapfileheader.size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + stride * image.GetHeight();
    bitmapinfoheader.width = image.GetWidth();
    bitmapinfoheader.height = image.GetHeight();
    bitmapinfoheader.size_image = stride * image.GetHeight();

    out.write(reinterpret_cast<char*>(&bitmapfileheader), sizeof(bitmapfileheader));
    out.write(reinterpret_cast<char*>(&bitmapinfoheader), sizeof(bitmapinfoheader));

    std::vector<char> buffer(bitmapinfoheader.width * 3);
    for (int y = image.GetHeight() - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < bitmapinfoheader.width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buffer.data(), buffer.size());
        out.write("\0\0\0", stride - bitmapinfoheader.width * 3); // Заполнение паддингом
    }

    return out.good();
}

// Функция для загрузки изображения
Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs) {
        return {};
    }

    BitmapFileHeader bitmapfileheader;
    BitmapInfoHeader bitmapinfoheader;

    ifs.read(reinterpret_cast<char*>(&bitmapfileheader), sizeof(bitmapfileheader));
    ifs.read(reinterpret_cast<char*>(&bitmapinfoheader), sizeof(bitmapinfoheader));

    if (bitmapfileheader.signature[0] != 'B' || bitmapfileheader.signature[1] != 'M') {
        throw runtime_error("Invalid BMP file");
    }

    Image result(bitmapinfoheader.width, bitmapinfoheader.height, Color::Black());
    std::vector<char> buffer(bitmapinfoheader.width * 3);

    for (int y = bitmapinfoheader.height - 1; y >= 0; --y) {
        ifs.read(buffer.data(), buffer.size());
        Color* line = result.GetLine(y);

        for (int x = 0; x < bitmapinfoheader.width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
        const int padding = GetBMPStride(bitmapinfoheader.width) - (bitmapinfoheader.width * 3);
        if (padding > 0) {
            ifs.ignore(padding);
        }
    }

    return result;
}

} // namespace img_lib
