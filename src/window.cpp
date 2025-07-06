#include "widget.h"
#include "window.h"

#include "util.h"

using namespace std::chrono;

Window::Window(GLFWwindow *window, Widget &root) : window_(window), root_(root) {
	glfwSetWindowUserPointer(window_, this);

#define win static_cast<Window*>(glfwGetWindowUserPointer(g))
	glfwSetKeyCallback(window_,             [](GLFWwindow *g, int key, int scancode, int action, int mods) { return win->key_cb(key, scancode, action, mods); });
	glfwSetCharCallback(window_,            [](GLFWwindow *g, unsigned int codepoint)                      { return win->char_cb(codepoint); });
	glfwSetCursorPosCallback(window_,       [](GLFWwindow *g, double xpos, double ypos)                    { return win->cursor_pos_cb(xpos, ypos); });
	glfwSetMouseButtonCallback(window_,     [](GLFWwindow *g, int button, int action, int mods)            { return win->mouse_button_cb(button, action, mods); });
	glfwSetScrollCallback(window_,          [](GLFWwindow *g, double xoffset, double yoffset)              { return win->scroll_cb(xoffset, yoffset); });
	glfwSetDropCallback(window_,            [](GLFWwindow *g, int path_count, const char* paths[])         { return win->drop_cb(path_count, paths); });
	glfwSetFramebufferSizeCallback(window_, [](GLFWwindow *g, int width, int height)                       { return win->frame_buffer_size_cb(width, height); });
	glfwSetWindowSizeCallback(window_,      [](GLFWwindow *g, int width, int height)                       { return win->window_size_cb(width, height); });
	glfwSetWindowRefreshCallback(window_,   [](GLFWwindow *g)                                              { return win->window_refresh_cb(); });
	// glfwSetMonitorCallback(                   [](GLFWmonitor* monitor, int event) { std::cout << "Monitor\n" << std::endl; });
#undef win
}

Window::~Window() {
	if (window_) {
		destroy();
	}
}

void Window::key_cb(int key, int scancode, int action, int mods) {
	key_mods_ = {mods};
	root_.key_cb(key, scancode, action, key_mods_);
}

void Window::char_cb(unsigned int codepoint) {
	root_.char_cb(codepoint, key_mods_);
}

void Window::cursor_pos_cb(double xpos, double ypos) {
	mouse_ = {xpos, ypos};
	root_.cursor_pos_cb(mouse_);
}

void Window::mouse_button_cb(int button, int action, int mods) {
	key_mods_ = {mods};
	root_.mouse_button_cb(mouse_, button, action, key_mods_);
}

void Window::scroll_cb(double xoffset, double yoffset) {
	root_.scroll_cb({xoffset, yoffset}, key_mods_);
}

void Window::drop_cb(int path_count, const char* paths[]) {
	root_.drop_cb(path_count, paths);
}

void Window::frame_buffer_size_cb(int width, int height) {
	glViewport(0, 0, width, height);
	root_.resize({0, 0}, {width, height});
}

void Window::window_size_cb(int width, int height) {
}

void Window::window_refresh_cb() {
	draw();
	glFinish();
}

void Window::destroy() {
	glfwDestroyWindow(window_);
	window_ = nullptr;
}

void Window::swap_buffers() const {
	glfwSwapBuffers(window_);
}

bool Window::should_close() const {
	return glfwWindowShouldClose(window_);
}

void Window::toggle_fullscreen() {
	if (fullscreen_) {
		glfwSetWindowMonitor(window_, nullptr, 0, 0, root_.size_.x, root_.size_.y, GLFW_DONT_CARE);
	} else {
		GLFWmonitor *monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);
		glfwSetWindowMonitor(window_, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	fullscreen_ = !fullscreen_;
}

bool Window::draw() const {
	bool tree_dirty = false;
	{
		// Timeit t("Update tree");
		tree_dirty = root_.do_update();
	}
	if (tree_dirty) {
		// Timeit t("Draw tree");
		root_.draw();
		swap_buffers();
	} else {
		// std::cout << "No changes in the tree, skipping draw." << std::endl;
	}
	return tree_dirty;
}

void Window::send_event() {
	// // glfwPostEmptyEvent();
	// std::lock_guard lock(event_mtx_);
	// event_pending_ = true;
	// event_cv_.notify_one();
	// // event_pending_.test_and_set();
}

// void Window::wait_events() {
// 	std::unique_lock lock(event_mtx_);
// 	event_cv_.wait_for(lock, 16ms, [this] { return event_pending_; });
// 	event_pending_ = false;
// 	// event_pending_.wait_for
// }
