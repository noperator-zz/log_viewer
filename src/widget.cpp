#include <typeinfo>
#include <iostream>
#include "widget.h"
#include <GLFW/glfw3.h>
#include "window.h"

using namespace glm;


Widget::Scissor::Scissor(Widget *widget) {
	if (!nesting_) {
		glEnable(GL_SCISSOR_TEST);
	}
	nesting_++;
	auto pos = widget->pos();
	auto size = widget->size();
	glScissor(pos.x, pos.y, size.x, size.y);
}

Widget::Scissor::~Scissor() {
	nesting_--;
	assert(nesting_ >= 0);
	if (nesting_ == 0) {
		glDisable(GL_SCISSOR_TEST);
	}
}

Widget::Widget() {
	char buf[128];
	sprintf(buf, "%s<%p>", typeid(*this).name(), this);
	name_ = std::string(buf);
}

Widget::Widget(std::string_view name) : name_(name) {}

bool Widget::mouse_button_cb(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	// bool has_focus = true;

	// // TODO if a child is pressed, then parent becomes un-hovered, this will prevent the child from becoming released
	// if (!state_.hovered && !state_.l_pressed && !state_.r_pressed && !state_.m_pressed) {
	// 	return false; // Ignore mouse events if not hovered or pressed
	// }

	bool handled_by_child = false;

	for (auto child : children_) {
		if (child->mouse_button_cb(mouse, button, action, mods)) {
			handled_by_child = true;
		}
		// if (child->state_.hovered) {
		// 	has_focus = false;
		// }
	}

	if (action != GLFW_PRESS) {
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
	}

	if (handled_by_child) {
		return true; // If a child handled the event, stop further processing
	}

	// if (!has_focus) {
	// 	// A child has focus, so we don't handle the event here
	// 	return false;
	// }

	if (action == GLFW_PRESS && state_.hovered) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				state_.l_pressed = true;
				pressed_mouse_pos_ = mouse;
				pressed_pos_ = pos_;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				state_.r_pressed = true;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				state_.m_pressed = true;
				break;
			default:
				return false; // Unsupported button
		}
	}

	return on_mouse_button(mouse, button, action, mods);
}

bool Widget::cursor_pos_cb(ivec2 mouse) {
	for (auto child : children_) {
		// TODO only call if child hovered (or pressed?) and handle return value
		child->cursor_pos_cb(mouse);
	}

	bool hovered = mouse.x >= pos_.x && mouse.x < pos_.x + size_.x &&
		mouse.y >= pos_.y && mouse.y < pos_.y + size_.y;

	if (hovered) {
		mouse_pos_ = mouse;
	}

	if (state_.l_pressed) {
		auto mouse_offset = mouse - pressed_mouse_pos_;
		auto self_offset = pos_ - pressed_pos_;
		auto offset = mouse_offset - self_offset;
		on_drag(offset);
	}

	if (hovered) {
		if (!state_.hovered) {
			state_.hovered = true;
			std::cout << "H " << path() << std::endl;
			on_hover();
		}
	} else {
		if (state_.hovered) {
			state_.hovered = false;
			std::cout << "U " << path() << std::endl;
			on_unhover();
		}
	}

	on_cursor_pos(mouse);

	return state_.hovered; // Return true if hovered, false otherwise
}

bool Widget::key_cb(int key, int scancode, int action, Window::KeyMods mods) {
	for (auto child : children_) {
		if (child->key_cb(key, scancode, action, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_key(key, scancode, action, mods);
}

bool Widget::char_cb(unsigned int codepoint, Window::KeyMods mods) {
	for (auto child : children_) {
		if (child->char_cb(codepoint, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_char(codepoint, mods);
}

bool Widget::scroll_cb(ivec2 offset, Window::KeyMods mods) {
	for (auto child : children_) {
		if (child->scroll_cb(offset, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_scroll(offset, mods);
}

bool Widget::drop_cb(int path_count, const char* paths[]) {
	for (auto child : children_) {
		if (child->drop_cb(path_count, paths)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_drop(path_count, paths);
}

void Widget::draw_cb() {
	// glScissor(pos().x, pos().y, size().x, size().y);
	draw();
	//
	// for (auto child : children_) {
	// 	child->draw_cb();
	// }
}

Widget *Widget::parent() const {
	return parent_;
}

// Window *Widget::window() const {
// 	return window_;
// }


ivec2 Widget::mouse_pos() const {
	return mouse_pos_;
}

ivec2 Widget::pos() const {
	return pos_;
}

ivec2 Widget::size() const {
	return size_;
}

ivec2 Widget::rel_pos(ivec2 abs_pos) const {
	return abs_pos - pos_;
}

bool Widget::hovered() const {
	return state_.hovered;
}

bool Widget::pressed(int button) const {
	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			return state_.l_pressed;
		case GLFW_MOUSE_BUTTON_RIGHT:
			return state_.r_pressed;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			return state_.m_pressed;
		default:
			return false; // Unsupported button
	}
}

void Widget::add_child(Widget &child) {
	child.parent_ = this;
	// child->window_ = window_;
	children_.push_back(&child);
}
void Widget::remove_child(Widget &child) {
	child.parent_ = nullptr;
	// child->window_ = nullptr;
	std::erase(children_, &child);
}

// std::string_view Widget::type() const {
// 	return typeid(*this).name();
// }

std::string_view Widget::name() const {
	return name_;
}

std::string Widget::path() const {
	if (parent_) {
		return parent_->path() + "/" + std::string{name_};
	}
	return std::string{name_};
}

std::tuple<int, int, int, int> Widget::resize(ivec2 pos, ivec2 size) {
	if (parent_) {
		auto max_size = parent_->size() - (pos - parent_->pos());
		if (size.x <= 0) {
			size.x = parent_->size().x + size.x;
		}
		size.x = std::min(size.x, max_size.x);

		if (size.y <= 0) {
			size.y = parent_->size().y + size.y;
		}
		size.y = std::min(size.y, max_size.y);
	}
	pos_ = pos;
	size_ = size;
	_on_resize();
	return {pos.x, pos.y, size.x, size.y};
}

void Widget::_on_resize() {
	on_resize();
	for (auto child : children_) {
		child->_on_resize();
	}
}
