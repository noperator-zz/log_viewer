#include "widget.h"
#include "window.h"

#include "util.h"

using namespace std::chrono;
using namespace glm;

Window::Window(GLFWwindow *window, Widget &root) : window_(window), root_(root) {
	root_.window_ = this;
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

void Window::update_hovered() {
	// TODO maybe only run this when widgets have moved (resize()) instead of every time it's needed
	// TODO make this more efficient by re-checking the current hovered widget first (check if it or any of it's children are hovered)
	// TODO add_child and remove_child could mess with this?
	auto prev = hovered_;
	if (hovered_) {
		// Check if still hovered
		// if (!hovered_->hit_test(mouse_)) {
		// 	hovered_->state_.hovered = false;
		// 	std::cout << "U " << hovered_->path() << std::endl;
		// 	hovered_->unhover_cb();
		// 	// hovered_ = nullptr;
		// }
	}

	// Find the deepest hovered child
	hovered_ = root_.deepest_hit_child(mouse_);

	if (hovered_ != prev) {
		if (prev) {
			prev->hover_cb(false);
		}
		if (hovered_) {
			hovered_->hover_cb(true);
		}
	}

	// if (hovered_ && hovered_ != prev) {
	// 	hovered_->state_.hovered = true;
	// 	std::cout << "H " << hovered_->path() << std::endl;
	// 	hovered_->hover_cb();
	// }
}

void Window::key_cb(int key, int scancode, int action, int mods) {
	key_mods_ = {mods};

	Widget *w = key_active_ ? key_active_ : &root_;
	invoke_on(w, &Widget::key_cb, key, scancode, action, key_mods_);
}

void Window::char_cb(unsigned int codepoint) {
	Widget *w = key_active_ ? key_active_ : &root_;
	invoke_on(w, &Widget::char_cb, codepoint, key_mods_);
}

void Window::cursor_pos_cb(double xpos, double ypos) {
	mouse_ = {xpos, ypos};
	update_hovered();

	if (mouse_active_) {
		// Send the event to the active widget

		if (state_.l_pressed) {
			auto mouse_offset = mouse_ - pressed_mouse_pos_;
			auto self_offset = mouse_active_->pos_ - pressed_pos_;
			auto offset = mouse_offset - self_offset;
			mouse_active_->drag_cb(offset);
		}

		mouse_active_->cursor_pos_cb(mouse_);
	} else if (hovered_) {
		invoke_on(hovered_, &Widget::cursor_pos_cb, mouse_);
	}
}

void Window::mouse_button_cb(int button, int action, int mods) {
	key_mods_ = {mods};
	update_hovered();

	if (action == GLFW_PRESS) {
		assert(hovered_);
		if (!mouse_active_) {
			mouse_active_ = hovered_;
		}
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				state_.l_pressed = true;
				pressed_mouse_pos_ = mouse_;
				pressed_pos_ = mouse_active_->pos_;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				state_.r_pressed = true;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				state_.m_pressed = true;
				break;
			default:
				break;
		}
		mouse_active_ = invoke_on(mouse_active_, &Widget::mouse_button_cb, mouse_, button, action, key_mods_);

	} else if (action == GLFW_RELEASE) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				state_.l_pressed = false;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				state_.r_pressed = false;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				state_.m_pressed = false;
				break;
			default:
				break;
		}
		if (mouse_active_) {
			invoke_on(mouse_active_, &Widget::mouse_button_cb, mouse_, button, action, key_mods_);
			if (!state_.l_pressed &&
				!state_.r_pressed &&
				!state_.m_pressed) {
				mouse_active_ = nullptr;
			}
		}
	} else {
		assert(false);
	}
}

void Window::scroll_cb(double xoffset, double yoffset) {
	update_hovered();
	invoke_on(hovered_, &Widget::scroll_cb, ivec2{xoffset, yoffset}, key_mods_);
}

void Window::drop_cb(int path_count, const char* paths[]) {
	update_hovered();
	invoke_on(hovered_, &Widget::drop_cb, path_count, paths);
}

void Window::frame_buffer_size_cb(int width, int height) {
	fb_size_ = {width, height};
	glViewport(0, 0, width, height);
	root_.resize({0, 0}, fb_size_);
}

void Window::window_size_cb(int width, int height) {
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

void Window::set_key_focus(Widget *widget) {
	key_active_ = widget;
}

uvec2 Window::mouse() const {
	return mouse_;
}
