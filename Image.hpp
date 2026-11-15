#ifndef IMAGE_HPP
#define IMAGE_HPP



#include "Bmp.hpp"

#include <cstdint>
#include <cstdlib>  // std::abs
#include <string>
#include <vector>

class Image {
public:
    Image() = default;


    void read(const std::string& filename);

    void write(const std::string& filename) const;

  
    int32_t width()  const { return infoHeader_.biWidth; }

    int32_t height() const { return std::abs(infoHeader_.biHeight); }

    std::vector<std::vector<RGBTRIPLE>>& pixels() { return pixels_; }

    const std::vector<std::vector<RGBTRIPLE>>& pixels() const { return pixels_; }

    std::vector<std::vector<RGBTRIPLE>> getPixelsCopy() const;


    void setPixels(const std::vector<std::vector<RGBTRIPLE>>& newPixels);


    void printHistogram() const;

private:
    BITMAPFILEHEADER fileHeader_{};
    BITMAPINFOHEADER infoHeader_{};
    std::vector<std::vector<RGBTRIPLE>> pixels_;

    int paddingBytes() const {
        return (4 - (width() * static_cast<int32_t>(sizeof(RGBTRIPLE))) % 4) % 4;
    }
};

#endif
