#ifndef HARRIS_HPP
#define HARRIS_HPP

#include "image.hpp"
#include <vector>
#include <utility>
#include <limits>

struct Descriptor {
    std::pair<int, int> pos; // (row, col)
    int n;
    std::vector<double> data;
};

Descriptor describe_point(const Image& im, int r, int c);
Image mark_spot(const Image& im, std::pair<int, int> p, const std::vector<float>& color = {255.0f, 0.0f, 255.0f});
Image mark_corners(const Image& im, const std::vector<Descriptor>& d, int n);
Image smooth_image(const Image& im, float sigma);
Image structure_matrix(const Image& im, float sigma);
Image cornerness_response(const Image& S);
Image nms_image(const Image& im, int w);
std::vector<Descriptor> harris_corner_detector(const Image& im, float sigma, float thresh, int nms);
Image detect_and_draw_corners(const Image& im, float sigma, float thresh, int nms);

#endif // HARRIS_HPP
