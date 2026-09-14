#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

#include "image.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

Image Image::load(const std::string& filename) {
    int w, h, ch;
    unsigned char* data_ptr = stbi_load(filename.c_str(), &w, &h, &ch, 0);
    if (!data_ptr) {
        throw std::runtime_error("Failed to load image: " + filename);
    }

    Image img(h, w, ch);
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            for (int k = 0; k < ch; ++k) {
                img(r, c, k) = static_cast<float>(data_ptr[(r * w + c) * ch + k]);
            }
        }
    }
    stbi_image_free(data_ptr);
    return img;
}

bool Image::save(const std::string& filename) const {
    if (empty()) return false;

    std::vector<unsigned char> byte_data(rows * cols * channels);
    for (int i = 0; i < rows * cols * channels; ++i) {
        float val = std::round(data[i]);
        val = std::max(0.0f, std::min(255.0f, val));
        byte_data[i] = static_cast<unsigned char>(val);
    }

    std::string ext = "";
    size_t dot_pos = filename.find_last_of(".");
    if (dot_pos != std::string::npos) {
        ext = filename.substr(dot_pos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    }

    if (ext == "png") {
        return stbi_write_png(filename.c_str(), cols, rows, channels, byte_data.data(), cols * channels) != 0;
    } else if (ext == "jpg" || ext == "jpeg") {
        return stbi_write_jpg(filename.c_str(), cols, rows, channels, byte_data.data(), 90) != 0;
    } else {
        // Default to PNG
        return stbi_write_png(filename.c_str(), cols, rows, channels, byte_data.data(), cols * channels) != 0;
    }
}

Image Image::to_grayscale() const {
    Image gray(rows, cols, 1);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float sum = 0.0f;
            for (int k = 0; k < channels; ++k) {
                sum += (*this)(r, c, k);
            }
            gray(r, c, 0) = sum / static_cast<float>(channels);
        }
    }
    return gray;
}

Image Image::normalize_grid() const {
    float min_val = data[0];
    float max_val = data[0];
    for (float v : data) {
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
    }

    Image norm(rows, cols, channels);
    float range = (max_val - min_val > 1e-8f) ? (max_val - min_val) : 1.0f;
    for (size_t i = 0; i < data.size(); ++i) {
        norm.data[i] = (data[i] - min_val) / range;
    }
    return norm;
}

static inline int clamp(int val, int low, int high) {
    return std::max(low, std::min(high, val));
}

Image convolve2d(const Image& im, const std::vector<std::vector<float>>& kernel) {
    int kh = kernel.size();
    int kw = kernel[0].size();
    int r_offset = kh / 2;
    int c_offset = kw / 2;

    Image out(im.rows, im.cols, im.channels, 0.0f);

    for (int r = 0; r < im.rows; ++r) {
        for (int c = 0; c < im.cols; ++c) {
            for (int ch = 0; ch < im.channels; ++ch) {
                float val = 0.0f;
                for (int kr = 0; kr < kh; ++kr) {
                    for (int kc = 0; kc < kw; ++kc) {
                        int nr = clamp(r + kr - r_offset, 0, im.rows - 1);
                        int nc = clamp(c + kc - c_offset, 0, im.cols - 1);
                        val += kernel[kr][kc] * im(nr, nc, ch);
                    }
                }
                out(r, c, ch) = val;
            }
        }
    }
    return out;
}

Image gaussian_filter(const Image& im, float sigma) {
    if (sigma <= 0.0f) return im.copy();

    int radius = static_cast<int>(std::ceil(4.0f * sigma));
    if (radius < 1) radius = 1;
    int ksize = 2 * radius + 1;

    std::vector<float> kernel(ksize);
    float sum = 0.0f;
    for (int i = -radius; i <= radius; ++i) {
        float g = std::exp(-static_cast<float>(i * i) / (2.0f * sigma * sigma));
        kernel[i + radius] = g;
        sum += g;
    }
    for (int i = 0; i < ksize; ++i) kernel[i] /= sum;

    // Horizontal pass
    Image temp(im.rows, im.cols, im.channels);
    for (int r = 0; r < im.rows; ++r) {
        for (int c = 0; c < im.cols; ++c) {
            for (int ch = 0; ch < im.channels; ++ch) {
                float val = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    int nc = clamp(c + k, 0, im.cols - 1);
                    val += kernel[k + radius] * im(r, nc, ch);
                }
                temp(r, c, ch) = val;
            }
        }
    }

    // Vertical pass
    Image out(im.rows, im.cols, im.channels);
    for (int r = 0; r < im.rows; ++r) {
        for (int c = 0; c < im.cols; ++c) {
            for (int ch = 0; ch < im.channels; ++ch) {
                float val = 0.0f;
                for (int k = -radius; k <= radius; ++k) {
                    int nr = clamp(r + k, 0, im.rows - 1);
                    val += kernel[k + radius] * temp(nr, c, ch);
                }
                out(r, c, ch) = val;
            }
        }
    }

    return out;
}
