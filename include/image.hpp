#ifndef IMAGE_HPP
#define IMAGE_HPP

#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <iostream>

struct Image {
    int rows;
    int cols;
    int channels;
    std::vector<float> data; // Pixel values normalized to float [0, 255] or [0, 1]

    Image() : rows(0), cols(0), channels(0) {}
    Image(int r, int c, int ch) : rows(r), cols(c), channels(ch), data(r * c * ch, 0.0f) {}
    Image(int r, int c, int ch, float val) : rows(r), cols(c), channels(ch), data(r * c * ch, val) {}

    inline float& operator()(int r, int c, int ch = 0) {
        return data[(r * cols + c) * channels + ch];
    }

    inline const float& operator()(int r, int c, int ch = 0) const {
        return data[(r * cols + c) * channels + ch];
    }

    bool empty() const {
        return data.empty() || rows == 0 || cols == 0;
    }

    Image copy() const {
        Image img(rows, cols, channels);
        img.data = data;
        return img;
    }

    static Image load(const std::string& filename);
    bool save(const std::string& filename) const;

    Image to_grayscale() const;
    Image normalize_grid() const; // (img - min) / (max - min)
};

// Image processing helper routines
Image convolve2d(const Image& im, const std::vector<std::vector<float>>& kernel);
Image gaussian_filter(const Image& im, float sigma);

#endif // IMAGE_HPP
