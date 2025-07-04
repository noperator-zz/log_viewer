#include "widget.h"
#include "window.h"

#include "util.h"

using namespace std::chrono;

Window::Window(GLFWwindow *window, Widget *root) : window_(window), root_(root) {
	glfwSetWindowUserPointer(window_, this);

	glfwSetKeyCallback(window_,              key_cb);
	glfwSetCharCallback(window_,             char_cb);
	glfwSetCursorPosCallback(window_,        cursor_pos_cb);
	glfwSetMouseButtonCallback(window_,      mouse_button_cb);
	glfwSetScrollCallback(window_,           scroll_cb);
	glfwSetDropCallback(window_,             drop_cb);
	glfwSetFramebufferSizeCallback(window_,  frame_buffer_size_cb);
	glfwSetWindowSizeCallback(window_,       window_size_cb);
	glfwSetWindowRefreshCallback(window_,    window_refresh_cb);
	// glfwSetMonitorCallback(                   [](GLFWmonitor* monitor, int event) { std::cout << "Monitor\n" << std::endl; });
}

Window::~Window() {
	if (window_) {
		destroy();
	}
}

void Window::key_cb(GLFWwindow *glfw_window, int key, int scancode, int action, int mods) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->key_mods_ = {mods};
	window->root_->key_cb(key, scancode, action, window->key_mods_);
}

void Window::char_cb(GLFWwindow *glfw_window, unsigned int codepoint) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->char_cb(codepoint, window->key_mods_);
}

void Window::cursor_pos_cb(GLFWwindow *glfw_window, double xpos, double ypos) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->mouse_ = {xpos, ypos};
	window->root_->cursor_pos_cb(window->mouse_);
}

void Window::mouse_button_cb(GLFWwindow *glfw_window, int button, int action, int mods) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->key_mods_ = {mods};
	window->root_->mouse_button_cb(window->mouse_, button, action, window->key_mods_);
}

void Window::scroll_cb(GLFWwindow *glfw_window, double xoffset, double yoffset) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->scroll_cb({xoffset, yoffset}, window->key_mods_);
}

void Window::drop_cb(GLFWwindow *glfw_window, int path_count, const char* paths[]) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->drop_cb(path_count, paths);
}

void Window::frame_buffer_size_cb(GLFWwindow *glfw_window, int width, int height) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	glViewport(0, 0, width, height);
	window->root_->resize({0, 0}, {width, height});
}

void Window::window_size_cb(GLFWwindow *glfw_window, int width, int height) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
}

void Window::window_refresh_cb(GLFWwindow *glfw_window) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->draw();
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
		glfwSetWindowMonitor(window_, nullptr, 0, 0, root_->size_.x, root_->size_.y, GLFW_DONT_CARE);
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
		tree_dirty = root_->do_update();
	}
	if (tree_dirty) {
		// Timeit t("Draw tree");
		root_->draw();
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
