#include <typeinfo>
#include <iostream>
#include "widget.h"
#include <GLFW/glfw3.h>

#include "Tracy.hpp"
#include "util.h"
#include "window.h"

using namespace glm;


Widget::Scissor::Scissor(Widget *widget) {
	if (stack_.empty()) {
		glEnable(GL_SCISSOR_TEST);
	}
	auto window_h = widget->window()->fb_size_.y;
	auto pos = widget->pos();
	auto size = widget->size();
	stack_.emplace_back(pos.x, window_h - (pos.y + size.y), size.x, size.y);
	auto rect = stack_.back();
	glScissor(rect.x, rect.y, rect.z, rect.w);
}

Widget::Scissor::~Scissor() {
	if (!stack_.empty()) {
		auto rect = stack_.back();
		stack_.pop_back();
		glScissor(rect.x, rect.y, rect.z, rect.w);
	}
	if (stack_.empty()) {
		glDisable(GL_SCISSOR_TEST);
	}
}

Widget::Widget(Widget *parent) : parent_(parent), window_(parent ? parent->window_ : nullptr) {
	char buf[128];
	sprintf(buf, "%s<%p>", typeid(*this).name(), this);
	name_ = std::string(buf);
}

Widget::Widget(Widget *parent, std::string_view name) : parent_(parent), window_(parent ? parent->window_ : nullptr), name_(name) {}

Widget::Widget(Window &window, Widget *parent, std::string_view name) : parent_(parent), window_(&window), name_(name) {}

bool Widget::soil_if_handled(bool handled) {
	if (handled) {
		soil();
	}
	return handled;
}

void Widget::hover_cb(bool state) {
	on_hover(state);
}

bool Widget::mouse_button_cb(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	// for (auto child : children_) {
	// 	if (child->mouse_button_cb(mouse, button, action, mods)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_mouse_button(mouse, button, action, mods));
}

bool Widget::cursor_pos_cb(ivec2 mouse) {
	// for (auto child : children_) {
	// 	if (child->cursor_pos_cb(mouse)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_cursor_pos(mouse));
}

bool Widget::key_cb(int key, int scancode, int action, Window::KeyMods mods) {
	// for (auto child : children_) {
	// 	if (child->key_cb(key, scancode, action, mods)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_key(key, scancode, action, mods));
}

bool Widget::char_cb(unsigned int codepoint, Window::KeyMods mods) {
	// for (auto child : children_) {
	// 	if (child->char_cb(codepoint, mods)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_char(codepoint, mods));
}

bool Widget::scroll_cb(dvec2 offset, Window::KeyMods mods) {
	// for (auto child : children_) {
	// 	if (child->scroll_cb(offset, mods)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_scroll(offset, mods));
}

bool Widget::drop_cb(int path_count, const char* paths[]) {
	// for (auto child : children_) {
	// 	if (child->drop_cb(path_count, paths)) {
	// 		return true;
	// 	}
	// }
	return soil_if_handled(on_drop(path_count, paths));
}

// void Widget::hover_cb() {
// 	on_hover();
// }
//
// void Widget::unhover_cb() {
// 	on_unhover();
// }

void Widget::drag_cb(ivec2 offset) {
	on_drag(offset);
}

void Widget::soil() {
	dirty_ = true;
	Window::send_event();
}

bool Widget::do_update() {
	ZoneScopedN("do_update");
	ZoneTextF("do_update: %s", name_.c_str());
	bool tree_dirty = dirty_;
	if (tree_dirty) {
		dirty_ = false;
		update();
	}
	for (auto child : children_) {
		tree_dirty |= child->do_update();
	}
	return tree_dirty;
}

Window *Widget::window() {
	return window_;
}

ivec2 Widget::mouse_pos() const {
	return window_->mouse();
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
	return hit_test(window_->mouse());
	// return state_.hovered;
}

bool Widget::pressed(int button) const {
	if (window_->mouse_active_ != this) {
		return false; // Not the active widget
	}

	switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			return window_->state_.l_pressed;
		case GLFW_MOUSE_BUTTON_RIGHT:
			return window_->state_.r_pressed;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			return window_->state_.m_pressed;
		default:
			return false; // Unsupported button
	}
}

// const Window &Widget::window() const {
// 	return *window_;
// }

void Widget::add_child(Widget &child) {
	child.parent_ = this;
	// child.window_ = window_;
	children_.push_back(&child);
}
void Widget::remove_child(Widget &child) {
	child.parent_ = nullptr;
	// child.window_ = nullptr;
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
	dirty_ = true;
	_on_resize();
	return {pos.x, pos.y, size.x, size.y};
}

void Widget::_on_resize() {
	on_resize();
	for (auto child : children_) {
		child->_on_resize();
	}
}

bool Widget::hit_test(const uvec2 &point) const {
	return point.x >= pos_.x && point.x < pos_.x + size_.x &&
		   point.y >= pos_.y && point.y < pos_.y + size_.y;
}

Widget *Widget::deepest_hit_child(const uvec2 point) {
	if (!hit_test(point)) {
		return nullptr;
	}
	for (auto &child : children_) {
		if (Widget *w = child->deepest_hit_child(point)) {
			return w;
		}
	}
	return this;
}


void Widget::take_key_focus() {
	window()->set_key_focus(this);
}

void Widget::drop_key_focus() {
	window()->set_key_focus(parent_);
}
