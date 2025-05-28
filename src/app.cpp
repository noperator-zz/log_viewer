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

std::unique_ptr<App> App::app_ {};

App::App() : Widget(nullptr, {0, 0}, {0, 0}) {
}

void App::create(int argc, char *argv[]) {
    if (app_) {
        std::cerr << "App already created\n";
        return;
    }
    app_ = std::make_unique<App>();
    WidgetManager::set_root(app_.get());
    app_->start();

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            app_->add_file(argv[i]);
        }
    }

    app_->run();
}

int App::start() {
    Timeit init_timeit("Init");

    if (glfwInit() == 0) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    // const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    //
    // // Create a windowed fullscreen (borderless full screen) window
    // glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    // glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    // glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    // glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    // glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    //
    // fb_w = mode->width;
    // fb_h = mode->height;

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    window_ = glfwCreateWindow(800, 600, "Text Hello", nullptr, nullptr);
    // window_ = glfwCreateWindow(mode->width, mode->height, "Text Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window_);

    int fb_width, fb_height;
    glfwGetFramebufferSize(window_, &fb_width, &fb_height);
    fb_size_ = {fb_width, fb_height};
    resize({0, 0}, fb_size_);

    // glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetKeyCallback(window_,              static_key_cb);
    glfwSetCursorPosCallback(window_,        static_cursor_pos_cb);
    glfwSetMouseButtonCallback(window_,      static_mouse_button_cb);
    glfwSetScrollCallback(window_,           static_scroll_cb);
    glfwSetDropCallback(window_,             static_drop_cb);
    glfwSetFramebufferSizeCallback(window_,  static_frame_buffer_size_cb);
    glfwSetWindowSizeCallback(window_,       static_window_size_cb);
    glfwSetWindowRefreshCallback(window_,    static_window_refresh_cb);
    // glfwSetMonitorCallback(                   [](GLFWmonitor* monitor, int event) { std::cout << "Monitor\n" << std::endl; });

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
        std::cerr << "Failed to compile Text shader\n";
        return -1;
    }

    gp_shader_ = std::make_unique<GPShader>();
    if (gp_shader_->setup() != 0) {
        std::cerr << "Failed to compile GP shader\n";
        return -1;
    }

    return 0;
}

int App::add_file(const char *path) {
    auto view = std::make_unique<FileView>(*this, pos(), size(), path, *text_shader_.get(), *gp_shader_.get());
    {
        Timeit file_open_timeit("File Open");
        if (view->open() != 0) {
            std::cerr << "Failed to open file\n";
            return -1;
        }
    }
    // view->set_viewport({100, 100, screenWidth / 2, screenHeight / 2});
    view->set_viewport({0, 0, fb_size_});

    // TODO move
    text_shader_->set_viewport({0, 0, fb_size_});
    gp_shader_->set_viewport({0, 0, fb_size_});

    file_views_.emplace_back(std::move(view));
    add_child(file_views_.back().get());
    return 0;
}

FileView& App::active_file_view() {
    return *file_views_.front();
}

void App::static_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    app_->key_cb(window, key, scancode, action, mods);
}
void App::key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
        if (action == GLFW_PRESS) {
            shift_held_ = true;
        } else if (action == GLFW_RELEASE) {
            shift_held_ = false;
        }
    }
}

void App::static_cursor_pos_cb(GLFWwindow* window, double xpos, double ypos) {
    app_->cursor_pos_cb(window, xpos, ypos);
}
void App::cursor_pos_cb(GLFWwindow* window, double xpos, double ypos) {
    // mouse_ = {xpos, ypos};
    WidgetManager::handle_cursor_pos({xpos, ypos});
}


void App::static_mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
    app_->mouse_button_cb(window, button, action, mods);
}
void App::mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
    WidgetManager::handle_mouse_button(button, action, mods);
}

void App::static_scroll_cb(GLFWwindow* window, double xoffset, double yoffset) {
    app_->scroll_cb(window, xoffset, yoffset);
}
void App::scroll_cb(GLFWwindow* window, double xoffset, double yoffset) {
    if (shift_held_) {
        xoffset = yoffset;
        yoffset = 0;
    }
    active_file_view().scroll({xoffset, yoffset});
}

void App::static_drop_cb(GLFWwindow* window, int path_count, const char* paths[]) {
    app_->drop_cb(window, path_count, paths);
}
void App::drop_cb(GLFWwindow* window, int path_count, const char* paths[]) {
    for (int i = 0; i < path_count; i++) {
        add_file(paths[i]);
    }
}

void App::static_frame_buffer_size_cb(GLFWwindow* window, int width, int height) {
    app_->frame_buffer_size_cb(window, width, height);
}
void App::frame_buffer_size_cb(GLFWwindow* window, int width, int height) {
    resize({0, 0}, {width, height});
}

void App::static_window_size_cb(GLFWwindow* window, int width, int height) {
    app_->window_size_cb(window, width, height);
}
void App::window_size_cb(GLFWwindow* window, int width, int height) {
}

void App::static_window_refresh_cb(GLFWwindow* window) {
    app_->window_refresh_cb(window);
}
void App::window_refresh_cb(GLFWwindow* window) {
    draw();
    glFinish();
}


void App::on_cursor_pos(glm::ivec2 pos) {
    mouse_ = pos;
}

void App::on_resize() {
    fb_size_ = size();
    // active_file_view().set_viewport({0, 0, fb_size_});
}

void App::draw() {
    glClear(GL_COLOR_BUFFER_BIT);

    gp_shader_->clear();

    for (auto &file_view : file_views_) {
        file_view->draw();
    }

    gp_shader_->draw();

    glfwSwapBuffers(window_);
}

int App::run() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );


    auto last_stat = high_resolution_clock::now();
    size_t fps = 0;
    while (!glfwWindowShouldClose(window_)) {
        auto frame_start = high_resolution_clock::now();
        // Timeit frame("Frame");
        draw();
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
