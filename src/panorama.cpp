#include "panorama.hpp"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

Matrix make_translation_homography(double dr, double dc) {
    Matrix H = Matrix::identity(3);
    H(0, 2) = dr;
    H(1, 2) = dc;
    return H;
}

Image both_images(const Image& a, const Image& b) {
    int width = a.cols + b.cols;
    int height = std::max(a.rows, b.rows);
    int channels = std::max(a.channels, b.channels);

    Image both(height, width, channels, 0.0f);
    for (int r = 0; r < a.rows; ++r) {
        for (int c = 0; c < a.cols; ++c) {
            for (int k = 0; k < a.channels; ++k) {
                both(r, c, k) = a(r, c, k);
            }
        }
    }

    for (int r = 0; r < b.rows; ++r) {
        for (int c = 0; c < b.cols; ++c) {
            for (int k = 0; k < b.channels; ++k) {
                both(r, c + a.cols, k) = b(r, c, k);
            }
        }
    }
    return both;
}

Image draw_matches(const Image& a, const Image& b, const std::vector<Match>& matches, int inliers) {
    Image both = both_images(a, b);
    int n = static_cast<int>(matches.size());

    for (int i = 0; i < n; ++i) {
        int r1 = matches[i].p.first;
        int r2 = matches[i].q.first;
        int c1 = matches[i].p.second;
        int c2 = matches[i].q.second;

        int start_c = c1;
        int end_c = c2 + a.cols;
        int delta_c = end_c - start_c;

        if (delta_c == 0) continue;

        float green_ch = (i < inliers) ? 255.0f : 0.0f;
        float red_ch   = (i < inliers) ? 0.0f : 255.0f;

        for (int c = start_c; c <= end_c; ++c) {
            double t = static_cast<double>(c - start_c) / static_cast<double>(delta_c);
            int r = static_cast<int>(t * (r2 - r1) + r1);
            if (r >= 0 && r < both.rows && c >= 0 && c < both.cols) {
                if (both.channels >= 3) {
                    both(r, c, 0) = red_ch;
                    both(r, c, 1) = green_ch;
                    both(r, c, 2) = 0.0f;
                }
            }
        }
    }
    return both;
}

Image draw_inliers(const Image& a, const Image& b, const Matrix& H, const std::vector<Match>& matches, double thresh) {
    auto res = model_inliers(H, matches, thresh);
    return draw_matches(a, b, res.second, res.first);
}

Image find_and_draw_matches(const Image& a, const Image& b, float sigma, float thresh, int nms) {
    std::vector<Descriptor> ad = harris_corner_detector(a, sigma, thresh, nms);
    std::vector<Descriptor> bd = harris_corner_detector(b, sigma, thresh, nms);
    std::vector<Match> m = match_descriptors(ad, bd);

    Image ma = mark_corners(a, ad, static_cast<int>(ad.size()));
    Image mb = mark_corners(b, bd, static_cast<int>(bd.size()));
    return draw_matches(ma, mb, m, 0);
}

double l1_distance(const std::vector<double>& a, const std::vector<double>& b) {
    double dist = 0.0;
    size_t sz = std::min(a.size(), b.size());
    for (size_t i = 0; i < sz; ++i) {
        dist += std::abs(a[i] - b[i]);
    }
    return dist;
}

std::vector<Match> match_descriptors(const std::vector<Descriptor>& a, const std::vector<Descriptor>& b) {
    int an = static_cast<int>(a.size());
    int bn = static_cast<int>(b.size());
    std::vector<Match> matches;
    matches.reserve(an);

    for (int j = 0; j < an; ++j) {
        double best_dist = std::numeric_limits<double>::infinity();
        int bind = 0;
        for (int i = 0; i < bn; ++i) {
            double d = l1_distance(a[j].data, b[i].data);
            if (d < best_dist) {
                best_dist = d;
                bind = i;
            }
        }
        Match m;
        m.ai = j;
        m.bi = bind;
        m.p = a[j].pos;
        m.q = b[bind].pos;
        m.distance = best_dist;
        matches.push_back(m);
    }

    // Sort matches based on distance
    std::sort(matches.begin(), matches.end(), [](const Match& m1, const Match& m2) {
        return m1.distance < m2.distance;
    });

    // Injective filter: throw out matches to the same element in b
    std::vector<Match> filtered_matches;
    for (int i = 0; i < an; ++i) {
        bool same = false;
        for (int j = 0; j < an; ++j) {
            if (i != j && matches[j].bi == matches[i].bi) {
                same = true;
                break;
            }
        }
        if (!same) {
            filtered_matches.push_back(matches[i]);
        }
    }
    return filtered_matches;
}

std::pair<double, double> project_point(const Matrix& H, const std::pair<double, double>& p) {
    Matrix p_homo(3, 1);
    p_homo(0, 0) = p.first;
    p_homo(1, 0) = p.second;
    p_homo(2, 0) = 1.0;

    Matrix c = H * p_homo;
    double w = c(2, 0) + 1e-8;
    return {c(0, 0) / w, c(1, 0) / w};
}

double point_distance(const std::pair<double, double>& p, const std::pair<double, double>& q) {
    double dr = p.first - q.first;
    double dc = p.second - q.second;
    return std::sqrt(dr * dr + dc * dc);
}

std::pair<int, std::vector<Match>> model_inliers(const Matrix& H, const std::vector<Match>& matches, double thresh) {
    int count = 0;
    std::vector<Match> inliers;
    std::vector<Match> outliers;

    for (const auto& match : matches) {
        std::pair<double, double> p = {static_cast<double>(match.p.first), static_cast<double>(match.p.second)};
        std::pair<double, double> q = {static_cast<double>(match.q.first), static_cast<double>(match.q.second)};
        std::pair<double, double> proj = project_point(H, p);

        double dist = point_distance(proj, q);
        if (dist < thresh) {
            inliers.push_back(match);
            count++;
        } else {
            outliers.push_back(match);
        }
    }
    inliers.insert(inliers.end(), outliers.begin(), outliers.end());
    return {count, inliers};
}

std::vector<Match> randomize_matches(std::vector<Match> matches) {
    static std::mt19937 g(1337);
    int n = static_cast<int>(matches.size());
    for (int i = n - 1; i > 0; --i) {
        std::uniform_int_distribution<int> dist(0, i);
        int j = dist(g);
        std::swap(matches[i], matches[j]);
    }
    return matches;
}

Matrix compute_homography(const std::vector<Match>& matches, int n) {
    if (n < 4 || static_cast<int>(matches.size()) < n) {
        return Matrix(); // Invalid
    }

    Matrix M(n * 2, 8, 0.0);
    Matrix b(n * 2, 1, 0.0);

    for (int i = 0; i < n; ++i) {
        double r  = static_cast<double>(matches[i].p.first);
        double rp = static_cast<double>(matches[i].q.first);
        double c  = static_cast<double>(matches[i].p.second);
        double cp = static_cast<double>(matches[i].q.second);

        int h = i * 2;
        // Even row
        M(h, 0) = r;
        M(h, 1) = c;
        M(h, 2) = 1.0;
        M(h, 3) = 0.0;
        M(h, 4) = 0.0;
        M(h, 5) = 0.0;
        M(h, 6) = -r * rp;
        M(h, 7) = -c * rp;

        // Odd row
        M(h + 1, 0) = 0.0;
        M(h + 1, 1) = 0.0;
        M(h + 1, 2) = 0.0;
        M(h + 1, 3) = r;
        M(h + 1, 4) = c;
        M(h + 1, 5) = 1.0;
        M(h + 1, 6) = -r * cp;
        M(h + 1, 7) = -c * cp;

        b(h, 0) = rp;
        b(h + 1, 0) = cp;
    }

    Matrix a;
    if (!M.solve(b, a)) {
        return Matrix();
    }

    Matrix H(3, 3);
    H(0, 0) = a(0, 0); H(0, 1) = a(1, 0); H(0, 2) = a(2, 0);
    H(1, 0) = a(3, 0); H(1, 1) = a(4, 0); H(1, 2) = a(5, 0);
    H(2, 0) = a(6, 0); H(2, 1) = a(7, 0); H(2, 2) = 1.0;

    return H;
}

Matrix RANSAC(std::vector<Match> matches, double thresh, int k, int cutoff) {
    int best = 0;
    Matrix Hb = make_translation_homography(0.0, 0.0);
    int n = 4;

    if (static_cast<int>(matches.size()) < 4) {
        return Hb;
    }

    int total_matches = static_cast<int>(matches.size());
    // Require a meaningful cutoff threshold (e.g. at least 30% of matches or cutoff)
    int target_cutoff = std::max(cutoff, static_cast<int>(total_matches * 0.5));

    for (int i = 0; i < k; ++i) {
        matches = randomize_matches(matches);
        Matrix H = compute_homography(matches, n);
        if (H.rows == 0) continue;

        auto res = model_inliers(H, matches, thresh);
        int count = res.first;
        const auto& inlier_matches = res.second;

        if (count > best) {
            best = count;
            if (count >= n) {
                Matrix H_inliers = compute_homography(inlier_matches, count);
                if (H_inliers.rows == 3) {
                    Hb = H_inliers;
                }
            } else {
                Hb = H;
            }

            if (best >= target_cutoff) {
                std::cout << "Nb iterations ransac: " << i << " (early exit with " << best << " inliers)\n";
                return Hb;
            }
        }
    }
    std::cout << "RANSAC finished " << k << " iterations. Best inliers: " << best << " / " << total_matches << "\n";
    return Hb;
}

Image combine_images(const Image& a, const Image& b, const Matrix& H) {
    Matrix Hinv;
    try {
        Hinv = H.inv3x3();
    } catch (...) {
        std::cout << "Homography inversion failed, returning image a\n";
        return a.copy();
    }

    // Project corners of b into a coordinates
    auto c1 = project_point(Hinv, {0.0, 0.0});
    auto c2 = project_point(Hinv, {static_cast<double>(b.rows), 0.0});
    auto c3 = project_point(Hinv, {0.0, static_cast<double>(b.cols)});
    auto c4 = project_point(Hinv, {static_cast<double>(b.rows), static_cast<double>(b.cols)});

    double max_r = std::max({c1.first, c2.first, c3.first, c4.first});
    double max_c = std::max({c1.second, c2.second, c3.second, c4.second});
    double min_r = std::min({c1.first, c2.first, c3.first, c4.first});
    double min_c = std::min({c1.second, c2.second, c3.second, c4.second});

    int dr = static_cast<int>(std::min(0.0, min_r));
    int dc = static_cast<int>(std::min(0.0, min_c));
    int h = static_cast<int>(std::max(static_cast<double>(a.rows), max_r) - dr);
    int w = static_cast<int>(std::max(static_cast<double>(a.cols), max_c) - dc);

    if (w > 7000 || h > 7000) {
        std::cout << "output too big, stopping.\n";
        return a.copy();
    }

    Image c(h, w, a.channels, 0.0f);
    std::cout << "a shape: (" << a.rows << ", " << a.cols << ", " << a.channels << ")\n";

    // Paste image a into new image offset by dr, dc
    for (int k = 0; k < a.channels; ++k) {
        for (int j = 0; j < a.cols; ++j) {
            for (int i = 0; i < a.rows; ++i) {
                c(i - dr, j - dc, k) = a(i, j, k);
            }
        }
    }

    // Paste image b using bilinear interpolation
    for (int i = 0; i < c.rows; ++i) {
        for (int j = 0; j < c.cols; ++j) {
            auto p = project_point(H, {static_cast<double>(i + dr), static_cast<double>(j + dc)});
            double x = p.first;
            double y = p.second;

            int x1 = static_cast<int>(std::floor(x));
            int x2 = static_cast<int>(std::ceil(x));
            int y1 = static_cast<int>(std::floor(y));
            int y2 = static_cast<int>(std::ceil(y));

            if (x1 < 0 || x2 >= b.rows || y1 < 0 || y2 >= b.cols) {
                continue;
            }

            double denom_x = (x2 == x1) ? 1.0 : (x2 - x1);
            double denom_y = (y2 == y1) ? 1.0 : (y2 - y1);

            for (int k = 0; k < c.channels; ++k) {
                double b11 = b(x1, y1, k);
                double b21 = b(x2, y1, k);
                double b12 = b(x1, y2, k);
                double b22 = b(x2, y2, k);

                double fxy1 = (x2 - x) * b11 / denom_x + (x - x1) * b21 / denom_x;
                double fxy2 = (x2 - x) * b12 / denom_x + (x - x1) * b22 / denom_x;
                double fxy  = (y2 - y) * fxy1 / denom_y + (y - y1) * fxy2 / denom_y;

                if (fxy > 0.001) { // Only blend if b pixel is non-zero
                    if (c(i, j, k) == 0.0f) {
                        c(i, j, k) = static_cast<float>(fxy);
                    } else {
                        c(i, j, k) = (c(i, j, k) + static_cast<float>(fxy)) * 0.5f;
                    }
                }
            }
        }
    }
    return c;
}

Image panorama_image(const Image& a, const Image& b, float sigma, float thresh, int nms, double inlier_thresh, int iters, int cutoff) {
    std::vector<Descriptor> ad = harris_corner_detector(a, sigma, thresh, nms);
    std::vector<Descriptor> bd = harris_corner_detector(b, sigma, thresh, nms);
    std::vector<Match> m = match_descriptors(ad, bd);

    Matrix H = RANSAC(m, inlier_thresh, iters, cutoff);
    return combine_images(a, b, H);
}
