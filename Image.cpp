#include "Image.hpp"

#include <algorithm>  
#include <fstream>
#include <iomanip>    
#include <iostream>
#include <stdexcept>


void Image::read(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open input file: " + filename);
    }


    file.read(reinterpret_cast<char*>(&fileHeader_), sizeof(fileHeader_));
    file.read(reinterpret_cast<char*>(&infoHeader_), sizeof(infoHeader_));

    if (!file) {
        throw std::runtime_error("Error reading BMP headers from: " + filename);
    }

   
    if (fileHeader_.bfType != 0x4D42) {
        throw std::runtime_error("Not a BMP file (bad magic number): " + filename);
    }
    if (infoHeader_.biBitCount != 24) {
        throw std::runtime_error("Only 24-bit BMPs are supported: " + filename);
    }
    if (infoHeader_.biCompression != 0) {
        throw std::runtime_error("Only uncompressed BMPs are supported: " + filename);
    }

 
    const int32_t w = width();
    const int32_t h = height();
    const bool    topDown = (infoHeader_.biHeight < 0);
    const int     padding = paddingBytes();

    
    file.seekg(fileHeader_.bfOffBits, std::ios::beg);

    
    pixels_.assign(static_cast<std::size_t>(h),
                   std::vector<RGBTRIPLE>(static_cast<std::size_t>(w)));

    for (int32_t row = 0; row < h; ++row) {
      
        const int32_t targetRow = topDown ? row : (h - 1 - row);

        file.read(reinterpret_cast<char*>(
                      pixels_[static_cast<std::size_t>(targetRow)].data()),
                  w * static_cast<std::streamsize>(sizeof(RGBTRIPLE)));

        file.seekg(padding, std::ios::cur);
    }

    if (!file) {
        throw std::runtime_error("Error reading pixel data from: " + filename);
    }

  
    infoHeader_.biHeight = h;


    infoHeader_.biSize      = sizeof(BITMAPINFOHEADER);             
    fileHeader_.bfOffBits   = sizeof(BITMAPFILEHEADER)
                            + sizeof(BITMAPINFOHEADER);            
    infoHeader_.biSizeImage = static_cast<uint32_t>((w * 3 + padding) * h);
    fileHeader_.bfSize      = fileHeader_.bfOffBits + infoHeader_.biSizeImage;
}

void Image::write(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open output file: " + filename);
    }

    const int32_t w       = width();
    const int32_t h       = height();
    const int     padding = paddingBytes();

 
    BITMAPFILEHEADER fhOut = fileHeader_;
    BITMAPINFOHEADER ihOut = infoHeader_;

    ihOut.biSize      = sizeof(BITMAPINFOHEADER);              
    fhOut.bfOffBits   = sizeof(BITMAPFILEHEADER)
                      + sizeof(BITMAPINFOHEADER);                
    ihOut.biSizeImage = static_cast<uint32_t>((w * 3 + padding) * h);
    fhOut.bfSize      = fhOut.bfOffBits + ihOut.biSizeImage;

    file.write(reinterpret_cast<const char*>(&fhOut), sizeof(fhOut));
    file.write(reinterpret_cast<const char*>(&ihOut), sizeof(ihOut));



    const uint8_t pad     = 0x00;

    for (int32_t row = h - 1; row >= 0; --row) {
        file.write(reinterpret_cast<const char*>(
                       pixels_[static_cast<std::size_t>(row)].data()),
                   w * static_cast<std::streamsize>(sizeof(RGBTRIPLE)));

        for (int p = 0; p < padding; ++p) {
            file.write(reinterpret_cast<const char*>(&pad), 1);
        }
    }

    if (!file) {
        throw std::runtime_error("Error writing to output file: " + filename);
    }
}


std::vector<std::vector<RGBTRIPLE>> Image::getPixelsCopy() const {
    return pixels_;   
}

void Image::setPixels(const std::vector<std::vector<RGBTRIPLE>>& newPixels) {
    const auto h = static_cast<std::size_t>(height());
    const auto w = static_cast<std::size_t>(width());

    if (newPixels.size() != h) {
        throw std::invalid_argument(
            "setPixels: row count mismatch (expected " + std::to_string(h) +
            ", got " + std::to_string(newPixels.size()) + ")");
    }
    for (std::size_t r = 0; r < h; ++r) {
        if (newPixels[r].size() != w) {
            throw std::invalid_argument(
                "setPixels: column count mismatch at row " + std::to_string(r));
        }
    }

    pixels_ = newPixels;
}


void Image::printHistogram() const {

    std::vector<int> freq(256, 0);

    for (const auto& row : pixels_) {
        for (const auto& pixel : row) {
            int lum = (pixel.rgbtRed + pixel.rgbtGreen + pixel.rgbtBlue) / 3;
            ++freq[static_cast<std::size_t>(lum)];
        }
    }

    const int kNumBins  = 8;
    const int kBinWidth = 256 / kNumBins;  
    std::vector<int> bins(static_cast<std::size_t>(kNumBins), 0);

    for (int i = 0; i < 256; ++i) {
        bins[static_cast<std::size_t>(i / kBinWidth)] +=
            freq[static_cast<std::size_t>(i)];
    }


    int maxBin = *std::max_element(bins.begin(), bins.end());

   
    int peakIntensity = 0;
    for (int i = 1; i < 256; ++i) {
        if (freq[static_cast<std::size_t>(i)] >
            freq[static_cast<std::size_t>(peakIntensity)]) {
            peakIntensity = i;
        }
    }

  
    const int kBarMax = 40;
    std::cout << "\n--- Luminance Histogram ---\n";
    for (int b = 0; b < kNumBins; ++b) {
        int lo = b * kBinWidth;
        int hi = lo + kBinWidth - 1;
        int count  = bins[static_cast<std::size_t>(b)];
        int barLen = (maxBin > 0) ? (count * kBarMax / maxBin) : 0;

        std::cout << std::setw(3) << lo << "-" << std::setw(3) << hi << " | ";
        for (int i = 0; i < barLen; ++i) {
            std::cout << '#';
        }
        std::cout << "  (" << count << ")\n";
    }
    std::cout << "Peak intensity: " << peakIntensity << "\n\n";
}
