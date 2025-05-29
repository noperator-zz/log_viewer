#include "widget.h"
#include "window.h"

Window::Window(GLFWwindow *window, Widget *root) : window_(window), root_(root) {
	glfwSetWindowUserPointer(window_, this);

	glfwSetKeyCallback(window_,              key_cb);
	glfwSetCursorPosCallback(window_,        cursor_pos_cb);
	glfwSetMouseButtonCallback(window_,      mouse_button_cb);
	glfwSetScrollCallback(window_,           scroll_cb);
	glfwSetDropCallback(window_,             drop_cb);
	glfwSetFramebufferSizeCallback(window_,  frame_buffer_size_cb);
	glfwSetWindowSizeCallback(window_,       window_size_cb);
	glfwSetWindowRefreshCallback(window_,    window_refresh_cb);
	// glfwSetMonitorCallback(                   [](GLFWmonitor* monitor, int event) { std::cout << "Monitor\n" << std::endl; });
}

void Window::key_cb(GLFWwindow* glfw_window, int key, int scancode, int action, int mods) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
		if (action == GLFW_PRESS) {
			window->shift_held_ = true;
		} else if (action == GLFW_RELEASE) {
			window->shift_held_ = false;
		}
	}
	window->root_->key_cb(key, scancode, action, mods);
}

void Window::cursor_pos_cb(GLFWwindow* glfw_window, double xpos, double ypos) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->mouse_ = {xpos, ypos};
	window->root_->cursor_pos_cb(window->mouse_);
}

void Window::mouse_button_cb(GLFWwindow* glfw_window, int button, int action, int mods) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->mouse_button_cb(window->mouse_, button, action, mods);
}

void Window::scroll_cb(GLFWwindow* glfw_window, double xoffset, double yoffset) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->scroll_cb({xoffset, yoffset});
}

void Window::drop_cb(GLFWwindow* glfw_window, int path_count, const char* paths[]) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->root_->drop_cb(path_count, paths);
}

void Window::frame_buffer_size_cb(GLFWwindow* glfw_window, int width, int height) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	glViewport(0, 0, width, height);
	window->root_->resize({0, 0}, {width, height});
}

void Window::window_size_cb(GLFWwindow* glfw_window, int width, int height) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
}

void Window::window_refresh_cb(GLFWwindow* glfw_window) {
	auto window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
	window->draw();
	glFinish();
}

void Window::draw() {
	glEnable(GL_SCISSOR_TEST);
	root_->draw_cb();
	glDisable(GL_SCISSOR_TEST);
}
