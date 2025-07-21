#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "app_window.h"
#include "util.h"
#include "log.h"
#include "TracyOpenGL.hpp"

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

AppWindow::AppWindow(GLFWwindow *window) : Window(window, app_) {
}

AppWindow::~AppWindow() {
    destroy();
}

std::unique_ptr<AppWindow> AppWindow::create(int argc, char *argv[], int &err) {
    err = 0;
    int ret;

    Timeit init_timeit("Init");

    if (glfwInit() == 0) {
        std::cerr << "Failed to initialize GLFW\n";
        err = -1;
        return nullptr;
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

    // glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        err = -2;
        return nullptr;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    // glEnable(GL_ALPHA_TEST);
    // glAlphaFunc(GL_GREATER, 0.1f);

    auto win = std::unique_ptr<AppWindow>(new AppWindow(window));
    win->fb_size_ = {fb_width, fb_height};

    if ((ret = win->app_.start(argc, argv)) != 0) {
        std::cerr << "Failed to start app: " << ret << "\n";
        err = ret;
        return nullptr;
    }

    err = 0;
    return win;
}

void AppWindow::window_refresh_cb() {
    draw_cb();
    glFinish();
}

void AppWindow::draw_cb() {
    auto frame_start = high_resolution_clock::now();
    auto time_since_last_draw = duration_cast<milliseconds>(frame_start - last_draw_);

    // This is mostly so the cursor blinks smoothly
    bool force_draw = time_since_last_draw >= 50ms;

    bool did_draw = draw(force_draw);
    if (did_draw) {
        last_draw_ = frame_start;
        fps_++;
    }

    // auto frame_remain = 1000ms - duration_cast<milliseconds>(high_resolution_clock::now() - frame_start);
    // if (frame_remain > 0ms) {
    //     std::this_thread::sleep_for(frame_remain);
    // }

    auto stat_elapsed = duration_cast<milliseconds>(frame_start - last_stat_);
    if (stat_elapsed.count() > 1000) {
        fflush(stdout);
        logger << "FPS: " << fps_ << "\n";
        last_stat_ = frame_start;
        fps_ = 0;
    }

    // logger << "Wait event" << std::endl;
    fflush(stdout);
    // window_->wait_events();
    // TODO instead of glfwWaitEvents() and Window::send_event() to trigger an update,
    // hva e cv
    // glfwWaitEvents();
    // TODO limit to 60 FPS
}

int AppWindow::run() {
    // glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    // glDrawArrays(GL_TRIANGLES, 0, glyph_count * 6);
    // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    TracyGpuContext;
    app_.resize({0, 0}, fb_size_);

    // TODO limit framerate
    // TODO separate processing from rendering
    while (!should_close()) {
        glfwPollEvents();
        draw_cb();
    }

    glfwTerminate();
    return 0;
}
