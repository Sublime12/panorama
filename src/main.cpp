#include "image.hpp"
#include "harris.hpp"
#include "panorama.hpp"
#include "gui.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

void print_help(const char* prog) {
    std::cout << "Usage:\n"
              << "  " << prog << " --gui                    Launch Desktop GUI Interface (default if no args)\n"
              << "  " << prog << " --test-all               Run benchmark test suite\n"
              << "  " << prog << " --stitch <img1> <img2> --out <output> [options]\n"
              << "  " << prog << " --multi <img1> <img2> <img3> ... --out <output> [options]\n"
              << "  " << prog << " --matches <img1> <img2> --out <output> [options]\n\n"
              << "Options:\n"
              << "  --sigma <float>          Gaussian sigma for Harris (default: 2.0)\n"
              << "  --thresh <float>         Cornerness threshold for Harris (default: 0.004 / 0.0003)\n"
              << "  --nms <int>              Non-maximum suppression window (default: 3)\n"
              << "  --inlier-thresh <double> RANSAC inlier threshold in pixels (default: 1.0)\n"
              << "  --iters <int>            RANSAC maximum iterations (default: 2000)\n"
              << "  --cutoff <int>           RANSAC inlier cutoff to exit early (default: 15)\n"
              << "  --help                   Display this help message\n";
}

int run_test_all() {
    std::cout << "=== Running All Panorama Tests ===\n";

    std::string data_dir = "data/";
    std::string out_dir = "output/";

    std::cout << "\n[1/5] Processing Rainier 1 & 2...\n";
    Image im1 = Image::load(data_dir + "Rainier1.png");
    Image im2 = Image::load(data_dir + "Rainier2.png");
    
    Image matches_im1_im2 = find_and_draw_matches(im1, im2, 2.0f, 0.004f, 3);
    matches_im1_im2.save(out_dir + "matches_rainier_1_2.png");
    std::cout << "Saved " << out_dir << "matches_rainier_1_2.png\n";

    Image pan1_2 = panorama_image(im1, im2, 2.0f, 0.004f, 3, 1.0, 2000, 15);
    pan1_2.save(out_dir + "panorama_rainier_1_2.png");
    std::cout << "Saved " << out_dir << "panorama_rainier_1_2.png\n";

    std::cout << "\n[2/5] Processing Rainier 3 & 4...\n";
    Image im3 = Image::load(data_dir + "Rainier3.png");
    Image im4 = Image::load(data_dir + "Rainier4.png");

    Image matches_im3_im4 = find_and_draw_matches(im3, im4, 2.0f, 0.004f, 3);
    matches_im3_im4.save(out_dir + "matches_rainier_3_4.png");
    std::cout << "Saved " << out_dir << "matches_rainier_3_4.png\n";

    Image pan3_4 = panorama_image(im3, im4, 2.0f, 0.004f, 3, 1.0, 2000, 15);

    std::cout << "\n[3/5] Combining Rainier (1+2) with (3+4)...\n";
    Image pan1_4 = panorama_image(pan1_2, pan3_4, 2.0f, 0.004f, 3, 1.0, 2000, 15);
    pan1_4.save(out_dir + "panorama_rainier_1_2_3_4.png");
    std::cout << "Saved " << out_dir << "panorama_rainier_1_2_3_4.png\n";

    std::cout << "\n[4/5] Adding Rainier 5...\n";
    Image im5 = Image::load(data_dir + "Rainier5.png");
    Image pan1_5 = panorama_image(pan1_4, im5, 2.0f, 0.004f, 3, 1.0, 2000, 15);
    pan1_5.save(out_dir + "panorama_rainier_1_2_3_4_5.png");
    std::cout << "Saved " << out_dir << "panorama_rainier_1_2_3_4_5.png\n";

    std::cout << "\n[5/5] Processing UQAM images...\n";
    Image im_uqam1 = Image::load(data_dir + "uqam-1.png");
    Image im_uqam2 = Image::load(data_dir + "uqam-2.png");
    Image matches_uqam1_2 = find_and_draw_matches(im_uqam1, im_uqam2, 2.0f, 0.004f, 3);
    matches_uqam1_2.save(out_dir + "matches_uqam_1_2.png");
    std::cout << "Saved " << out_dir << "matches_uqam_1_2.png\n";

    Image pan_uqam = panorama_image(im_uqam1, im_uqam2, 2.0f, 0.0003f, 3, 1.0, 2000, 15);
    pan_uqam.save(out_dir + "panorama_uqam_1_2.png");
    std::cout << "Saved " << out_dir << "panorama_uqam_1_2.png\n";

    std::cout << "\n=== All Tests Completed Successfully! ===\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return run_gui();
    }

    std::string mode = "";
    std::vector<std::string> input_imgs;
    std::string output_path = "output_panorama.png";

    float sigma = 2.0f;
    float thresh = 0.004f;
    int nms = 3;
    double inlier_thresh = 1.0;
    int iters = 2000;
    int cutoff = 15;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--gui") {
            return run_gui();
        } else if (arg == "--test-all") {
            return run_test_all();
        } else if (arg == "--stitch" || arg == "--matches" || arg == "--multi") {
            mode = arg;
        } else if (arg == "--out") {
            if (i + 1 < argc) {
                output_path = argv[++i];
            }
        } else if (arg == "--sigma") {
            if (i + 1 < argc) sigma = std::stof(argv[++i]);
        } else if (arg == "--thresh") {
            if (i + 1 < argc) thresh = std::stof(argv[++i]);
        } else if (arg == "--nms") {
            if (i + 1 < argc) nms = std::stoi(argv[++i]);
        } else if (arg == "--inlier-thresh") {
            if (i + 1 < argc) inlier_thresh = std::stod(argv[++i]);
        } else if (arg == "--iters") {
            if (i + 1 < argc) iters = std::stoi(argv[++i]);
        } else if (arg == "--cutoff") {
            if (i + 1 < argc) cutoff = std::stoi(argv[++i]);
        } else {
            if (arg.find("--") != 0) {
                input_imgs.push_back(arg);
            }
        }
    }

    if (mode == "--matches") {
        if (input_imgs.size() < 2) {
            std::cerr << "Error: --matches requires 2 input images.\n";
            return 1;
        }
        std::cout << "Loading " << input_imgs[0] << " and " << input_imgs[1] << "...\n";
        Image a = Image::load(input_imgs[0]);
        Image b = Image::load(input_imgs[1]);
        Image res = find_and_draw_matches(a, b, sigma, thresh, nms);
        if (res.save(output_path)) {
            std::cout << "Matches saved to " << output_path << "\n";
        } else {
            std::cerr << "Failed to save matches to " << output_path << "\n";
            return 1;
        }
    } else if (mode == "--stitch") {
        if (input_imgs.size() < 2) {
            std::cerr << "Error: --stitch requires 2 input images.\n";
            return 1;
        }
        std::cout << "Stitching " << input_imgs[0] << " and " << input_imgs[1] << "...\n";
        Image a = Image::load(input_imgs[0]);
        Image b = Image::load(input_imgs[1]);
        Image pan = panorama_image(a, b, sigma, thresh, nms, inlier_thresh, iters, cutoff);
        if (pan.save(output_path)) {
            std::cout << "Panorama saved to " << output_path << "\n";
        } else {
            std::cerr << "Failed to save panorama to " << output_path << "\n";
            return 1;
        }
    } else if (mode == "--multi") {
        if (input_imgs.size() < 2) {
            std::cerr << "Error: --multi requires at least 2 input images.\n";
            return 1;
        }
        std::cout << "Stitching " << input_imgs.size() << " images into panorama...\n";
        Image pan = Image::load(input_imgs[0]);
        for (size_t idx = 1; idx < input_imgs.size(); ++idx) {
            std::cout << "Adding " << input_imgs[idx] << "...\n";
            Image next_img = Image::load(input_imgs[idx]);
            pan = panorama_image(pan, next_img, sigma, thresh, nms, inlier_thresh, iters, cutoff);
        }
        if (pan.save(output_path)) {
            std::cout << "Multi-image panorama saved to " << output_path << "\n";
        } else {
            std::cerr << "Failed to save panorama to " << output_path << "\n";
            return 1;
        }
    } else {
        print_help(argv[0]);
        return 1;
    }

    return 0;
}
