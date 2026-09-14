#include "harris.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

Descriptor describe_point(const Image& im, int r, int c) {
    int rad = 2;
    Descriptor d;
    d.pos = {r, c};
    d.n = (2 * rad + 1) * (2 * rad + 1) * im.channels;
    d.data.resize(d.n, 0.0);

    std::vector<double> cval(im.channels);
    for (int k = 0; k < im.channels; ++k) {
        cval[k] = im(r, c, k);
    }

    int r0 = (r - rad > 0) ? (r - rad) : 0;
    int r1 = (r + rad + 1 < im.rows) ? (r + rad + 1) : (im.rows - 1);
    int c0 = (c - rad > 0) ? (c - rad) : 0;
    int c1 = (c + rad + 1 < im.cols) ? (c + rad + 1) : (im.cols - 1);

    int idx = 0;
    for (int row = r0; row < r1; ++row) {
        for (int col = c0; col < c1; ++col) {
            for (int k = 0; k < im.channels; ++k) {
                if (idx < d.n) {
                    d.data[idx++] = static_cast<double>(im(row, col, k)) - cval[k];
                }
            }
        }
    }
    return d;
}

Image mark_spot(const Image& im, std::pair<int, int> p, const std::vector<float>& color) {
    Image m = im.copy();
    int r = p.first;
    int c = p.second;

    for (int i = -9; i <= 9; ++i) {
        if (r + i >= 0 && r + i < m.rows && c >= 0 && c < m.cols) {
            for (int k = 0; k < std::min(m.channels, static_cast<int>(color.size())); ++k) {
                m(r + i, c, k) = color[k];
            }
        }
        if (r >= 0 && r < m.rows && c + i >= 0 && c + i < m.cols) {
            for (int k = 0; k < std::min(m.channels, static_cast<int>(color.size())); ++k) {
                m(r, c + i, k) = color[k];
            }
        }
    }
    return m;
}

Image mark_corners(const Image& im, const std::vector<Descriptor>& d, int n) {
    Image m = im.copy();
    int count = std::min(n, static_cast<int>(d.size()));
    for (int i = 0; i < count; ++i) {
        m = mark_spot(m, d[i].pos);
    }
    return m;
}

Image smooth_image(const Image& im, float sigma) {
    return gaussian_filter(im, sigma);
}

Image structure_matrix(const Image& im, float sigma) {
    std::vector<std::vector<float>> Sx = {
        {1.0f, 0.0f, -1.0f},
        {2.0f, 0.0f, -2.0f},
        {1.0f, 0.0f, -1.0f}
    };
    std::vector<std::vector<float>> Sy = {
        { 1.0f,  2.0f,  1.0f},
        { 0.0f,  0.0f,  0.0f},
        {-1.0f, -2.0f, -1.0f}
    };

    Image dx = convolve2d(im, Sx);
    Image dy = convolve2d(im, Sy);

    Image dx2(im.rows, im.cols, 1);
    Image dy2(im.rows, im.cols, 1);
    Image dxdy(im.rows, im.cols, 1);

    for (int r = 0; r < im.rows; ++r) {
        for (int c = 0; c < im.cols; ++c) {
            float vx = dx(r, c, 0);
            float vy = dy(r, c, 0);
            dx2(r, c, 0) = vx * vx;
            dy2(r, c, 0) = vy * vy;
            dxdy(r, c, 0) = vx * vy;
        }
    }

    Image A = smooth_image(dx2, sigma);
    Image B = smooth_image(dy2, sigma);
    Image C = smooth_image(dxdy, sigma);

    Image S(im.rows, im.cols, 3);
    for (int r = 0; r < im.rows; ++r) {
        for (int c = 0; c < im.cols; ++c) {
            S(r, c, 0) = A(r, c, 0);
            S(r, c, 1) = B(r, c, 0);
            S(r, c, 2) = C(r, c, 0);
        }
    }
    return S;
}

Image cornerness_response(const Image& S) {
    Image R(S.rows, S.cols, 1);
    float alpha = 0.06f;

    for (int r = 0; r < S.rows; ++r) {
        for (int c = 0; c < S.cols; ++c) {
            float A = S(r, c, 0);
            float B = S(r, c, 1);
            float C = S(r, c, 2);

            float det = A * B - C * C;
            float trace = A + B;
            R(r, c, 0) = det - alpha * (trace * trace);
        }
    }
    return R;
}

Image nms_image(const Image& im, int w) {
    Image r_img = im.copy();
    float neg_inf = -std::numeric_limits<float>::infinity();

    for (int i = 0; i < im.rows; ++i) {
        for (int j = 0; j < im.cols; ++j) {
            float pixel = im(i, j, 0);
            bool lessThan = false;

            int min_k = std::max(0, i - w);
            int max_k = std::min(im.rows - 1, i + w);
            int min_l = std::max(0, j - w);
            int max_l = std::min(im.cols - 1, j + w);

            for (int k = min_k; k <= max_k; ++k) {
                for (int l = min_l; l <= max_l; ++l) {
                    if (pixel < im(k, l, 0)) {
                        lessThan = true;
                        r_img(i, j, 0) = neg_inf;
                        break;
                    }
                }
                if (lessThan) break;
            }
        }
    }
    return r_img;
}

std::vector<Descriptor> harris_corner_detector(const Image& im, float sigma, float thresh, int nms) {
    Image img = im.to_grayscale();
    img = img.normalize_grid();

    Image S = structure_matrix(img, sigma);
    Image R = cornerness_response(S);
    Image Rnms = nms_image(R, nms);

    std::vector<Descriptor> descriptors;
    for (int i = 0; i < Rnms.rows; ++i) {
        for (int j = 0; j < Rnms.cols; ++j) {
            if (Rnms(i, j, 0) > thresh) {
                descriptors.push_back(describe_point(im, i, j));
            }
        }
    }
    return descriptors;
}

Image detect_and_draw_corners(const Image& im, float sigma, float thresh, int nms) {
    std::vector<Descriptor> d = harris_corner_detector(im, sigma, thresh, nms);
    return mark_corners(im, d, static_cast<int>(d.size()));
}
