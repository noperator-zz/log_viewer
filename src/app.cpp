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
#include "util.h"
#include "file_view.h"
#include "../shaders/text_shader.h"
#include "util.h"
#include "window.h"
#include "log.h"
#include "stripe_shader.h"

using namespace std::chrono;

static void log_cb(const char *data, size_t length) {
    static auto const start = system_clock::now();
    // <timestamp> <thread_id> <file>:<line> <function_name> <message>
    auto now = system_clock::now();
    auto now_s = duration_cast<seconds>(now - start);
    auto now_ms = duration_cast<milliseconds>(now - start) % 1000;
    auto thread_id = std::this_thread::get_id();
    std::cout << "[" << now_s.count() << "." << now_ms.count() << "] "
              << "[" << thread_id << "] "
              << data << std::endl;
}

LogStream logger {log_cb};

App::App() : Widget("App") {
}

App::~App() {
    window_->destroy();
}

void App::create(int argc, char *argv[]) {
    if (app_) {
        std::cerr << "App already created\n";
        return;
    }
    app_ = std::unique_ptr<App>(new App());
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
    GLFWwindow *window = glfwCreateWindow(800, 600, "Text Hello", nullptr, nullptr);
    // window = glfwCreateWindow(mode->width, mode->height, "Text Hello", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    int fb_width, fb_height;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    fb_size_ = {fb_width, fb_height};

    // glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    // glEnable(GL_ALPHA_TEST);
// glAlphaFunc(GL_GREATER, 0.1f);

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

    window_ = std::make_unique<Window>(window, this);
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
        window_->toggle_fullscreen();
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
    fb_size_ = size();
    GPShader::set_viewport(fb_size_);
	TextShader::globals.set_viewport(fb_size_);
    StripeShader::globals.set_viewport(fb_size_);
    if (file_views_.empty()) {
        return; // No file views to resize
    }

    active_file_view().resize({0, 0}, fb_size_ - glm::ivec2{0, 0});
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

int App::run() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    resize({0, 0}, fb_size_);

    auto last_stat = high_resolution_clock::now();
    size_t fps = 0;
    // TODO limit framerate
    // TODO separate processing from rendering
    while (!window_->should_close()) {
        glfwPollEvents();

        auto frame_start = high_resolution_clock::now();
        fps += window_->draw();

        // auto frame_remain = 1000ms - duration_cast<milliseconds>(high_resolution_clock::now() - frame_start);
        // if (frame_remain > 0ms) {
        //     std::this_thread::sleep_for(frame_remain);
        // }

        auto stat_elapsed = duration_cast<milliseconds>(frame_start - last_stat);
        if (stat_elapsed.count() > 1000) {
            fflush(stdout);
            logger << "FPS: " << fps << "\n";
            last_stat = frame_start;
            fps = 0;
        }

        // logger << "Wait event" << std::endl;
        fflush(stdout);
        // window_->wait_events();
        // TODO instead of glfwWaitEvents() and Window::send_event() to trigger an update,
        // hva e cv
        // glfwWaitEvents();
        // TODO limit to 60 FPS
    }

    glfwTerminate();
    return 0;
}
