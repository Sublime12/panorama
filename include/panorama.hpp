#ifndef PANORAMA_HPP
#define PANORAMA_HPP

#include "image.hpp"
#include "matrix.hpp"
#include "harris.hpp"
#include <vector>
#include <utility>

struct Match {
    int ai;
    int bi;
    std::pair<int, int> p; // (r1, c1) in image a
    std::pair<int, int> q; // (r2, c2) in image b
    double distance;
};

Matrix make_translation_homography(double dr, double dc);
Image both_images(const Image& a, const Image& b);
Image draw_matches(const Image& a, const Image& b, const std::vector<Match>& matches, int inliers);
Image draw_inliers(const Image& a, const Image& b, const Matrix& H, const std::vector<Match>& matches, double thresh);
Image find_and_draw_matches(const Image& a, const Image& b, float sigma = 2.0f, float thresh = 3.0f, int nms = 3);
double l1_distance(const std::vector<double>& a, const std::vector<double>& b);
std::vector<Match> match_descriptors(const std::vector<Descriptor>& a, const std::vector<Descriptor>& b);

std::pair<double, double> project_point(const Matrix& H, const std::pair<double, double>& p);
double point_distance(const std::pair<double, double>& p, const std::pair<double, double>& q);
std::pair<int, std::vector<Match>> model_inliers(const Matrix& H, const std::vector<Match>& matches, double thresh);
std::vector<Match> randomize_matches(std::vector<Match> matches);
Matrix compute_homography(const std::vector<Match>& matches, int n);
Matrix RANSAC(std::vector<Match> matches, double thresh, int k, int cutoff);
Image combine_images(const Image& a, const Image& b, const Matrix& H);
Image panorama_image(const Image& a, const Image& b, float sigma = 2.0f, float thresh = 0.0003f, int nms = 3, double inlier_thresh = 1.0, int iters = 2000, int cutoff = 15);

#endif // PANORAMA_HPP
