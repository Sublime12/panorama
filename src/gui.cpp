#include "gui.hpp"
#include "image.hpp"
#include "harris.hpp"
#include "panorama.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <cmath>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#endif

template<typename T>
static T clamp_val(T val, T min_val, T max_val) {
    return std::max(min_val, std::min(max_val, val));
}

// OpenGL Texture cache structure
struct TextureCache {
    GLuint id = 0;
    int width = 0;
    int height = 0;

    void update(const Image& img) {
        if (img.empty()) return;

        if (id == 0) {
            glGenTextures(1, &id);
        }

        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        int w = img.cols;
        int h = img.rows;
        int ch = img.channels;

        std::vector<unsigned char> rgba(w * h * 4, 255);
        for (int r = 0; r < h; ++r) {
            for (int c = 0; c < w; ++c) {
                int idx = (r * w + c) * 4;
                if (ch == 1) {
                    unsigned char val = static_cast<unsigned char>(clamp_val(img(r, c, 0), 0.0f, 255.0f));
                    rgba[idx + 0] = val;
                    rgba[idx + 1] = val;
                    rgba[idx + 2] = val;
                    rgba[idx + 3] = 255;
                } else {
                    rgba[idx + 0] = static_cast<unsigned char>(clamp_val(img(r, c, 0), 0.0f, 255.0f));
                    rgba[idx + 1] = static_cast<unsigned char>(clamp_val(img(r, c, 1), 0.0f, 255.0f));
                    rgba[idx + 2] = static_cast<unsigned char>(clamp_val(img(r, c, 2), 0.0f, 255.0f));
                    rgba[idx + 3] = (ch >= 4) ? static_cast<unsigned char>(clamp_val(img(r, c, 3), 0.0f, 255.0f)) : 255;
                }
            }
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        width = w;
        height = h;
    }

    void destroy() {
        if (id != 0) {
            glDeleteTextures(1, &id);
            id = 0;
            width = 0;
            height = 0;
        }
    }
};

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

static void apply_custom_dark_theme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;

    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(9.0f, 5.0f);
    style.ItemSpacing = ImVec2(9.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(7.0f, 5.0f);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.ScrollbarSize = 14.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.94f, 0.96f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.52f, 0.55f, 0.58f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.11f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.15f, 0.16f, 0.19f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.29f, 0.34f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.30f, 0.34f, 0.40f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.11f, 0.13f, 0.75f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.31f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.36f, 0.40f, 0.46f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.44f, 0.49f, 0.56f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.35f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.35f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.45f, 0.76f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.20f, 0.46f, 0.76f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.54f, 0.88f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.32f, 0.62f, 0.96f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.20f, 0.25f, 0.32f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.33f, 0.42f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.32f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.35f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.45f, 0.76f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.35f, 0.68f, 0.98f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.45f, 0.76f, 1.00f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.33f, 0.42f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.46f, 0.76f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.18f, 0.22f, 0.28f, 1.00f);
}

int run_gui() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    // GL 3.3 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1400, 900, "Panorama Generator C++ (Dear ImGui)", NULL, NULL);
    if (!window) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    apply_custom_dark_theme();

    // Load web-style TTF typography (Roboto 15.5px body, 17.5px headers)
    ImFont* font_regular = io.Fonts->AddFontFromFileTTF("third_party/fonts/Roboto-Regular.ttf", 15.5f);
    ImFont* font_bold    = io.Fonts->AddFontFromFileTTF("third_party/fonts/Roboto-Medium.ttf", 17.5f);
    if (font_regular == nullptr) {
        font_regular = io.Fonts->AddFontDefault();
        font_bold = font_regular;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State Variables
    float sigma = 2.0f;
    float thresh = 0.004f;
    int nms = 3;
    double inlier_thresh = 1.0;
    int iters = 2000;
    int cutoff = 15;

    char img1_path[256] = "data/Rainier1.png";
    char img2_path[256] = "data/Rainier2.png";
    char multi_imgs[512] = "data/Rainier1.png, data/Rainier2.png, data/Rainier3.png, data/Rainier4.png, data/Rainier5.png";
    char out_path[256] = "output/panorama_gui_out.png";

    Image loaded_img1, loaded_img2;
    Image result_img;

    TextureCache tex_img1, tex_img2, tex_result;

    std::stringstream log_stream;
    log_stream << "[System] Panorama GUI initialized.\n";
    log_stream << "[System] Ready to load images or run benchmarks.\n";

    double last_proc_time_ms = 0.0;
    std::string status_msg = "Idle";

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Fullscreen dockable area or layout
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
        ImGui::Begin("Panorama Generator Studio", nullptr, 
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Split Layout: Left Controls (400px), Right Viewport (Remaining)
        float left_panel_width = 440.0f;
        float top_height = (float)display_h - 220.0f;

        // --- LEFT PANEL: CONTROL CENTER ---
        ImGui::BeginChild("LeftPanel", ImVec2(left_panel_width, top_height), true);

        ImGui::PushFont(font_bold);
        ImGui::TextColored(ImVec4(0.35f, 0.68f, 0.98f, 1.0f), "CONTROL CENTER");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTabBar("ModeTabBar")) {

            // TAB 1: 2-IMAGE STITCH & MATCHES
            if (ImGui::BeginTabItem("Stitch / Matches")) {
                ImGui::Spacing();
                ImGui::TextUnformatted("Input Images");
                ImGui::InputText("Image 1", img1_path, IM_ARRAYSIZE(img1_path));
                ImGui::InputText("Image 2", img2_path, IM_ARRAYSIZE(img2_path));

                ImGui::Spacing();
                if (ImGui::Button("Quick Preset: Rainier 1 & 2", ImVec2(-1, 0))) {
                    snprintf(img1_path, sizeof(img1_path), "data/Rainier1.png");
                    snprintf(img2_path, sizeof(img2_path), "data/Rainier2.png");
                }
                if (ImGui::Button("Quick Preset: UQAM 1 & 2", ImVec2(-1, 0))) {
                    snprintf(img1_path, sizeof(img1_path), "data/uqam-1.png");
                    snprintf(img2_path, sizeof(img2_path), "data/uqam-2.png");
                    thresh = 0.0003f;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::PushFont(font_bold);
                ImGui::TextColored(ImVec4(0.35f, 0.68f, 0.98f, 1.0f), "Harris Corner Detection");
                ImGui::PopFont();
                ImGui::SliderFloat("Sigma (Blur)", &sigma, 0.5f, 8.0f, "%.1f");
                ImGui::DragFloat("Threshold", &thresh, 0.0001f, 0.0001f, 0.05f, "%.4f");
                ImGui::SliderInt("NMS Window", &nms, 1, 15);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::PushFont(font_bold);
                ImGui::TextColored(ImVec4(0.35f, 0.68f, 0.98f, 1.0f), "RANSAC Homography");
                ImGui::PopFont();
                float inlier_f = (float)inlier_thresh;
                if (ImGui::SliderFloat("Inlier Thresh (px)", &inlier_f, 0.1f, 10.0f, "%.1f")) {
                    inlier_thresh = (double)inlier_f;
                }
                ImGui::SliderInt("Iterations", &iters, 100, 10000);
                ImGui::SliderInt("Inlier Cutoff", &cutoff, 5, 100);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::InputText("Output Path", out_path, IM_ARRAYSIZE(out_path));

                ImGui::Spacing();
                if (ImGui::Button("Find & Draw Matches", ImVec2(-1, 35))) {
                    try {
                        status_msg = "Running Feature Matching...";
                        log_stream << "[Process] Loading " << img1_path << " & " << img2_path << "...\n";
                        loaded_img1 = Image::load(img1_path);
                        loaded_img2 = Image::load(img2_path);
                        tex_img1.update(loaded_img1);
                        tex_img2.update(loaded_img2);

                        auto start = std::chrono::high_resolution_clock::now();
                        result_img = find_and_draw_matches(loaded_img1, loaded_img2, sigma, thresh, nms);
                        auto end = std::chrono::high_resolution_clock::now();
                        last_proc_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

                        tex_result.update(result_img);
                        log_stream << "[Success] Matches drawn in " << last_proc_time_ms << " ms. Output size: "
                                   << result_img.cols << "x" << result_img.rows << "\n";
                        status_msg = "Matches Ready";
                    } catch (const std::exception& e) {
                        log_stream << "[Error] " << e.what() << "\n";
                        status_msg = "Error Occurred";
                    }
                }

                if (ImGui::Button("Stitch 2 Images", ImVec2(-1, 35))) {
                    try {
                        status_msg = "Stitching 2 Images...";
                        log_stream << "[Process] Stitching " << img1_path << " & " << img2_path << "...\n";
                        loaded_img1 = Image::load(img1_path);
                        loaded_img2 = Image::load(img2_path);
                        tex_img1.update(loaded_img1);
                        tex_img2.update(loaded_img2);

                        auto start = std::chrono::high_resolution_clock::now();
                        result_img = panorama_image(loaded_img1, loaded_img2, sigma, thresh, nms, inlier_thresh, iters, cutoff);
                        auto end = std::chrono::high_resolution_clock::now();
                        last_proc_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

                        tex_result.update(result_img);
                        log_stream << "[Success] Stitching completed in " << last_proc_time_ms << " ms. Result size: "
                                   << result_img.cols << "x" << result_img.rows << "\n";
                        status_msg = "Panorama Ready";
                    } catch (const std::exception& e) {
                        log_stream << "[Error] " << e.what() << "\n";
                        status_msg = "Error Occurred";
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 2: MULTI-IMAGE PANORAMA
            if (ImGui::BeginTabItem("Multi-Image")) {
                ImGui::Spacing();
                ImGui::TextUnformatted("Comma-separated Image Paths:");
                ImGui::InputTextMultiline("Images", multi_imgs, IM_ARRAYSIZE(multi_imgs), ImVec2(-1, 60));

                ImGui::Spacing();
                if (ImGui::Button("Preset: Rainier 1 to 5", ImVec2(-1, 0))) {
                    snprintf(multi_imgs, sizeof(multi_imgs),
                             "data/Rainier1.png, data/Rainier2.png, data/Rainier3.png, data/Rainier4.png, data/Rainier5.png");
                }

                ImGui::Spacing();
                if (ImGui::Button("Stitch Multi-Image Sequence", ImVec2(-1, 38))) {
                    try {
                        std::vector<std::string> paths;
                        std::stringstream ss(multi_imgs);
                        std::string item;
                        while (std::getline(ss, item, ',')) {
                            // Trim whitespace
                            item.erase(0, item.find_first_not_of(" \t\n\r"));
                            item.erase(item.find_last_not_of(" \t\n\r") + 1);
                            if (!item.empty()) paths.push_back(item);
                        }

                        if (paths.size() < 2) {
                            log_stream << "[Error] At least 2 images required for multi-stitching.\n";
                        } else {
                            log_stream << "[Process] Starting sequential stitch of " << paths.size() << " images...\n";
                            status_msg = "Multi-Stitching in progress...";

                            auto start = std::chrono::high_resolution_clock::now();
                            Image current_pan = Image::load(paths[0]);
                            for (size_t i = 1; i < paths.size(); ++i) {
                                log_stream << " Adding (" << i + 1 << "/" << paths.size() << ") " << paths[i] << "...\n";
                                Image next_im = Image::load(paths[i]);
                                current_pan = panorama_image(current_pan, next_im, sigma, thresh, nms, inlier_thresh, iters, cutoff);
                            }
                            auto end = std::chrono::high_resolution_clock::now();
                            last_proc_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

                            result_img = current_pan;
                            tex_result.update(result_img);
                            log_stream << "[Success] Multi-image panorama created in " << last_proc_time_ms << " ms.\n";
                            status_msg = "Multi-Panorama Ready";
                        }
                    } catch (const std::exception& e) {
                        log_stream << "[Error] " << e.what() << "\n";
                        status_msg = "Error Occurred";
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 3: BENCHMARK SUITE
            if (ImGui::BeginTabItem("Benchmark Suite")) {
                ImGui::Spacing();
                ImGui::TextWrapped("Run the complete Rainier and UQAM benchmark pipeline automatically.");
                ImGui::Spacing();

                if (ImGui::Button("Run All Benchmark Tests", ImVec2(-1, 40))) {
                    try {
                        log_stream << "[Benchmark] Executing full test suite...\n";
                        auto start = std::chrono::high_resolution_clock::now();

                        Image im1 = Image::load("data/Rainier1.png");
                        Image im2 = Image::load("data/Rainier2.png");
                        Image pan1_2 = panorama_image(im1, im2, 2.0f, 0.004f, 3, 1.0, 2000, 15);

                        Image im3 = Image::load("data/Rainier3.png");
                        Image im4 = Image::load("data/Rainier4.png");
                        Image pan3_4 = panorama_image(im3, im4, 2.0f, 0.004f, 3, 1.0, 2000, 15);

                        Image pan1_4 = panorama_image(pan1_2, pan3_4, 2.0f, 0.004f, 3, 1.0, 2000, 15);
                        Image im5 = Image::load("data/Rainier5.png");
                        result_img = panorama_image(pan1_4, im5, 2.0f, 0.004f, 3, 1.0, 2000, 15);

                        auto end = std::chrono::high_resolution_clock::now();
                        last_proc_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

                        tex_result.update(result_img);
                        log_stream << "[Benchmark] Completed Rainier 1-5 in " << last_proc_time_ms << " ms!\n";
                        status_msg = "Benchmark Finished";
                    } catch (const std::exception& e) {
                        log_stream << "[Error] " << e.what() << "\n";
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (!result_img.empty()) {
            if (ImGui::Button("Save Current Result Image", ImVec2(-1, 32))) {
                if (result_img.save(out_path)) {
                    log_stream << "[Save] Saved image successfully to: " << out_path << "\n";
                } else {
                    log_stream << "[Error] Failed to save image to: " << out_path << "\n";
                }
            }
        }

        ImGui::EndChild();

        ImGui::SameLine();

        // --- RIGHT PANEL: VIEWPORT CANVAS ---
        ImGui::BeginChild("RightPanel", ImVec2(0, top_height), true);

        ImGui::PushFont(font_bold);
        ImGui::TextColored(ImVec4(0.35f, 0.68f, 0.98f, 1.0f), "VIEWPORT CANVAS");
        ImGui::PopFont();
        ImGui::SameLine();
        ImGui::TextDisabled("| Status: %s | Last Render Time: %.1f ms", status_msg.c_str(), last_proc_time_ms);
        ImGui::Separator();

        if (ImGui::BeginTabBar("ViewportTabs")) {
            if (ImGui::BeginTabItem("Result Output")) {
                if (tex_result.id != 0) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float aspect = (float)tex_result.width / (float)tex_result.height;
                    float draw_w = avail.x;
                    float draw_h = draw_w / aspect;

                    if (draw_h > avail.y) {
                        draw_h = avail.y;
                        draw_w = draw_h * aspect;
                    }

                    ImGui::Text("Resolution: %d x %d px", tex_result.width, tex_result.height);
                    ImGui::Image((ImTextureID)(intptr_t)tex_result.id, ImVec2(draw_w, draw_h));
                } else {
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 100);
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  No result image rendered yet.\n  Run an action on the left panel to preview results here.");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input Image 1")) {
                if (tex_img1.id != 0) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float aspect = (float)tex_img1.width / (float)tex_img1.height;
                    float draw_w = avail.x;
                    float draw_h = draw_w / aspect;
                    if (draw_h > avail.y) {
                        draw_h = avail.y;
                        draw_w = draw_h * aspect;
                    }
                    ImGui::Text("Resolution: %d x %d px", tex_img1.width, tex_img1.height);
                    ImGui::Image((ImTextureID)(intptr_t)tex_img1.id, ImVec2(draw_w, draw_h));
                } else {
                    ImGui::TextDisabled("Image 1 not loaded.");
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input Image 2")) {
                if (tex_img2.id != 0) {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    float aspect = (float)tex_img2.width / (float)tex_img2.height;
                    float draw_w = avail.x;
                    float draw_h = draw_w / aspect;
                    if (draw_h > avail.y) {
                        draw_h = avail.y;
                        draw_w = draw_h * aspect;
                    }
                    ImGui::Text("Resolution: %d x %d px", tex_img2.width, tex_img2.height);
                    ImGui::Image((ImTextureID)(intptr_t)tex_img2.id, ImVec2(draw_w, draw_h));
                } else {
                    ImGui::TextDisabled("Image 2 not loaded.");
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::EndChild();

        // --- BOTTOM PANEL: LOG CONSOLE ---
        ImGui::SetCursorPosY(top_height + 10);
        ImGui::BeginChild("BottomConsole", ImVec2(0, 0), true);
        ImGui::PushFont(font_bold);
        ImGui::TextColored(ImVec4(0.35f, 0.68f, 0.98f, 1.0f), "CONSOLE LOGS");
        ImGui::PopFont();
        ImGui::SameLine();
        if (ImGui::Button("Clear Log")) {
            log_stream.str("");
            log_stream.clear();
        }
        ImGui::Separator();
        ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextUnformatted(log_stream.str().c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.10f, 0.11f, 0.12f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup textures & context
    tex_img1.destroy();
    tex_img2.destroy();
    tex_result.destroy();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
