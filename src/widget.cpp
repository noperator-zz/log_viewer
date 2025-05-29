#include <GLFW/glfw3.h>
#include "widget.h"
#include "window.h"

using namespace glm;

bool Widget::mouse_button_cb(ivec2 mouse, int button, int action, int mods) {
	if (!state_.hovered && !state_.pressed) {
		return false; // Ignore mouse events if not hovered or pressed
	}

	for (auto child : children_) {
		if (child->mouse_button_cb(mouse, button, action, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			if (!state_.pressed) {
				// Only trigger on_press if not already pressed
				state_.pressed = true;
				pressed_mouse_pos_ = mouse;
				pressed_pos_ = pos_;
				on_press();
			}
		} else if (action == GLFW_RELEASE) {
			if (state_.pressed) {
				// Only trigger on_release if it was pressed
				state_.pressed = false;
				on_release();
			}
		}
	}
	return true; // Indicate that the event was handled
}

bool Widget::cursor_pos_cb(ivec2 mouse) {
	for (auto child : children_) {
		child->cursor_pos_cb(mouse);
	}

	on_cursor_pos(mouse);
	if (state_.pressed) {
		auto mouse_offset = mouse - pressed_mouse_pos_;
		auto self_offset = pos_ - pressed_pos_;
		auto offset = mouse_offset - self_offset;
		on_drag(offset);
	}

	if (mouse.x >= pos_.x && mouse.x < pos_.x + size_.x &&
	    mouse.y >= pos_.y && mouse.y < pos_.y + size_.y) {
		if (!state_.hovered) {
			state_.hovered = true;
			on_hover();
		}
	} else {
		if (state_.hovered) {
			state_.hovered = false;
			on_unhover();
		}
	}
	return state_.hovered; // Return true if hovered, false otherwise
}

bool Widget::key_cb(int key, int scancode, int action, int mods) {
	for (auto child : children_) {
		if (child->key_cb(key, scancode, action, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_key(key, scancode, action, mods);
}

bool Widget::scroll_cb(ivec2 offset) {
	for (auto child : children_) {
		if (child->scroll_cb(offset)) {
			return true; // If a child handled the event, stop further processing
		}
	}
	return on_scroll(offset);
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
	glScissor(pos().x, pos().y, size().x, size().y);
	draw();

	for (auto child : children_) {
		child->draw_cb();
	}
}

Widget *Widget::parent() const {
	return parent_;
}

// Window *Widget::window() const {
// 	return window_;
// }

ivec2 Widget::pos() const {
	return pos_;
}

ivec2 Widget::size() const {
	return size_;
}

bool Widget::hovered() const {
	return state_.hovered;
}

bool Widget::pressed() const {
	return state_.pressed;
}

void Widget::add_child(Widget *child) {
	child->parent_ = this;
	// child->window_ = window_;
	children_.push_back(child);
}
void Widget::remove_child(Widget *child) {
	child->parent_ = nullptr;
	// child->window_ = nullptr;
	std::erase(children_, child);
}

void Widget::resize(ivec2 pos, ivec2 size) {
	pos_ = pos;
	size_ = size;
	_on_resize();
}

void Widget::_on_resize() {
	if (parent_) {
		// pos_.x = std::max(pos_.x, parent_->pos().x);
		// pos_.y = std::max(pos_.y, parent_->pos().y);
		//
		// pos_.x = std::min(pos_.x, parent_->pos().x + parent_->size().x - size_.x);
		// pos_.y = std::min(pos_.y, parent_->pos().y + parent_->size().y - size_.y);
	}
	on_resize();
	for (auto child : children_) {
		child->_on_resize();
	}
}
