#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#include "app.h"
#include "font.h"
#include "File.h"
#include "parser.h"
#include "util.h"
#include "file_view.h"
#include "text_shader.h"
#include "util.h"

using namespace std::chrono;

int App::start() {
    Timeit init_timeit("Init");

    if (glfwInit() == 0) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(screenWidth, screenHeight, "Text Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    // glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (Font::init()) {
        std::cerr << "Failed to initialize FreeType\n";
        return -1;
    }

    init_timeit.~Timeit();

    font_ = std::make_unique<Font>(26,
        "C:/Windows/Fonts/Consola.ttf",
        "C:/Windows/Fonts/Consolab.ttf",
        "C:/Windows/Fonts/Consolai.ttf",
        "C:/Windows/Fonts/Consolaz.ttf"
    );

    {
        Timeit load_timeit("Font Load");
        if (font_->load()) {
            std::cerr << "Failed to load font\n";
            return -1;
        }
    }

    text_shader_ = std::make_unique<TextShader>(*font_.get());
    if (text_shader_->setup() != 0) {
        std::cerr << "Failed to compile shader\n";
        return -1;
    }

    return 0;
}

int App::add_file(const char *path) {
    auto view = std::make_unique<FileView>(path, *text_shader_.get());
    {
        Timeit file_open_timeit("File Open");
        if (view->open() != 0) {
            std::cerr << "Failed to open file\n";
            return -1;
        }
    }
    // view->set_viewport({100, 100, screenWidth / 2, screenHeight / 2});
    view->set_viewport({0, 0, screenWidth, screenHeight});

    file_views_.emplace_back(std::move(view));
    return 0;
}

int App::run() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );


    auto last_stat = high_resolution_clock::now();
    size_t fps = 0;
    while (!glfwWindowShouldClose(window)) {
        auto frame_start = high_resolution_clock::now();
        // Timeit frame("Frame");
        glClear(GL_COLOR_BUFFER_BIT);

        for (auto &file_view : file_views_) {
            file_view->draw();
        }

        glfwSwapBuffers(window);
        // frame.stop();
        glfwPollEvents();

        fps++;

        // auto frame_remain = 1000ms - duration_cast<milliseconds>(high_resolution_clock::now() - frame_start);
        // if (frame_remain > 0ms) {
        //     std::this_thread::sleep_for(frame_remain);
        // }

        auto stat_elapsed = duration_cast<milliseconds>(frame_start - last_stat);
        if (stat_elapsed.count() > 1000) {
            std::cout << "FPS: " << fps << "\n";
            last_stat = frame_start;
            fps = 0;
        }
    }

    glfwTerminate();
    return 0;
}
