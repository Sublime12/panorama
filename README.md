# Panorama Image Stitching in C++14

A standalone, zero-dependency C++14 implementation of Harris Corner Detection, RANSAC homography estimation, and image panorama stitching.

## Project Structure

```
panorama_cpp/
├── CMakeLists.txt     # CMake build configuration
├── Makefile           # Make build configuration
├── README.md          # Project documentation
├── .gitignore         # Git ignore file
├── include/           # Header files
│   ├── harris.hpp     # Harris corner detection declarations
│   ├── image.hpp      # Image buffer & processing routines
│   ├── matrix.hpp     # Matrix algebra & linear solver
│   └── panorama.hpp   # Homography, RANSAC & blending declarations
├── src/               # Source implementation files
│   ├── harris.cpp
│   ├── image.cpp
│   ├── main.cpp       # CLI Application
│   ├── matrix.cpp
│   └── panorama.cpp
├── third_party/       # Single-header libraries
│   └── stb/           # stb_image.h & stb_image_write.h
├── data/              # Test image datasets (Rainier & UQAM)
└── output/            # Generated panorama & match outputs
```

## Compilation

Build using standard `make`:
```bash
make
```

Or using `cmake`:
```bash
mkdir build && cd build
cmake ..
make
```

## Usage Examples

### 1. Run All Test Suites
Generates all Rainier and UQAM panoramas and match visualizations in `output/`:
```bash
./panorama_cli --test-all
```

### 2. Stitch Two Images
```bash
./panorama_cli --stitch data/Rainier1.png data/Rainier2.png --out output/panorama_1_2.png
```

### 3. Stitch Multiple Images Sequentially
```bash
./panorama_cli --multi data/Rainier1.png data/Rainier2.png data/Rainier3.png data/Rainier4.png --out output/panorama_rainier.png
```

### 4. Visualize Corner Matches
```bash
./panorama_cli --matches data/Rainier1.png data/Rainier2.png --out output/matches.png
```

### Options & Hyperparameters
- `--sigma <float>`: Gaussian blur standard deviation for Harris (default: `2.0`)
- `--thresh <float>`: Cornerness threshold for Harris (default: `0.004`)
- `--nms <int>`: Non-maximum suppression window size (default: `3`)
- `--inlier-thresh <double>`: RANSAC distance threshold in pixels for inliers (default: `1.0`)
- `--iters <int>`: RANSAC maximum iterations (default: `2000`)
- `--cutoff <int>`: RANSAC inlier count cutoff for early exit (default: `15`)
