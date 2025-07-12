#pragma once

#include <memory>
#include "app.h"
#include "window.h"

class AppWindow : public Window {
	App app_ {*this};
	std::chrono::time_point<std::chrono::high_resolution_clock> last_stat_ {std::chrono::high_resolution_clock::now()};
	std::chrono::time_point<std::chrono::high_resolution_clock> last_draw_ {std::chrono::high_resolution_clock::now()};
	size_t fps_ {};

	AppWindow(GLFWwindow *window);

	void window_refresh_cb() override;
	void draw_cb();


	AppWindow(const AppWindow &) = delete;
	AppWindow &operator=(const AppWindow &) = delete;
	AppWindow(AppWindow &&) = delete;
	AppWindow &operator=(AppWindow &&) = delete;

public:
	~AppWindow();
	[[nodiscard]] static std::unique_ptr<AppWindow> create(int argc, char *argv[], int &err);
	[[nodiscard]] int run();

	// using Window::destroy;
	// using Window::swap_buffers;
	using Window::toggle_fullscreen;
	// using Window::should_close;
	// using Window::draw;
};
