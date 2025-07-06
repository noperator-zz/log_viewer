#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>

#include "app.h"

#include "app_window.h"
#include "font.h"
#include "File.h"
#include "util.h"
#include "file_view.h"
#include "../shaders/text_shader.h"
#include "util.h"
#include "window.h"
#include "log.h"
#include "stripe_shader.h"

using namespace std::chrono;

App::App(AppWindow &window) : Widget(window, "App") {
}

App::~App() {
}

int App::start(int argc, char *argv[]) {
    Timeit init_timeit("Init");

    if (Font::init()) {
        std::cerr << "Failed to initialize FreeType\n";
        return -1;
    }

    init_timeit.~Timeit();

    font_ = std::make_unique<Font>(22,
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

    if (TextShader::init(*font_.get()) != 0) {
        std::cerr << "Failed to compile Text shader\n";
        return -1;
    }

    if (GPShader::init() != 0) {
        std::cerr << "Failed to compile GP shader\n";
        return -1;
    }

    if (StripeShader::init() != 0) {
        std::cerr << "Failed to compile Stripe shader\n";
        return -1;
    }

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            add_file(argv[i]);
        }
    }

    return 0;
}

int App::add_file(const char *path) {
    auto view = FileView::create(path);
    {
        Timeit file_open_timeit("View Open");
        if (view->open() != 0) {
            std::cerr << "Failed to open file\n";
            return -1;
        }
    }
    // view->set_viewport({100, 100, screenWidth / 2, screenHeight / 2});
    // view->set_viewport({0, 0, fb_size_});
    // view->resize({0, 0}, fb_size_);

    file_views_.emplace_back(std::move(view));
    add_child(*file_views_.back().get());
    return 0;
}

void App::file_worker() {
    // This function is currently not used, but can be implemented for background file processing
    // std::cout << "File worker started\n";
    // while (true) {
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
}

FileView& App::active_file_view() {
    return *file_views_.front();
}

bool App::on_key(int key, int scancode, int action, Window::KeyMods mods) {
    if (action == GLFW_RELEASE) {
        return false; // Ignore key release events
    }

    if (key == GLFW_KEY_F11) {
        static_cast<const AppWindow&>(window_).toggle_fullscreen();
        return true;
    }
    return false;
}

bool App::on_scroll(glm::ivec2 offset, Window::KeyMods mods) {
    if (mods.shift) {
        offset.x = offset.y;
        offset.y = 0;
    }
    active_file_view().scroll(offset);
    return true;
}

bool App::on_drop(int path_count, const char* paths[]) {
    for (int i = 0; i < path_count; i++) {
        add_file(paths[i]);
    }
    return true;
}

void App::on_resize() {
    GPShader::set_viewport(size());
	TextShader::globals.set_viewport(size());
    StripeShader::globals.set_viewport(size());
    if (file_views_.empty()) {
        return; // No file views to resize
    }

    active_file_view().resize({0, 0}, size() - glm::ivec2{0, 0});
}

void App::update() {
}

void App::draw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // GPShader::clear();

    for (auto &file_view : file_views_) {
        file_view->draw();
    }

    GPShader::draw();
}
