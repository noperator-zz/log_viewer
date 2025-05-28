#include "widget.h"

#include <GLFW/glfw3.h>

using namespace glm;

IWidget *WidgetManager::root_ {};
ivec2 WidgetManager::mouse_ {};

bool WidgetManager::handle_mouse_button(int button, int action, int mods) {
	if (root_) {
		return root_->handle_mouse_button(button, action, mods);
	}
	return false;
}

bool WidgetManager::handle_cursor_pos(ivec2 pos) {
	mouse_ = pos;
	if (root_) {
		return root_->handle_cursor_pos(pos);
	}
	return false;
}

void WidgetManager::set_root(IWidget *root) {
	root_ = root;
}

IWidget::IWidget(IWidget *parent, ivec2 pos, ivec2 size) : parent_(parent), pos_(pos), size_(size) {
}

bool IWidget::handle_mouse_button(int button, int action, int mods) {
	if (!state_.hovered && !state_.pressed) {
		return false; // Ignore mouse events if not hovered or pressed
	}

	for (auto child : children_) {
		if (child->handle_mouse_button(button, action, mods)) {
			return true; // If a child handled the event, stop further processing
		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			if (!state_.pressed) {
				// Only trigger on_press if not already pressed
				state_.pressed = true;
				pressed_mouse_pos_ = WidgetManager::mouse_;
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

bool IWidget::handle_cursor_pos(ivec2 mouse) {
	for (auto child : children_) {
		child->handle_cursor_pos(mouse);
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

IWidget *IWidget::parent() const {
	return parent_;
}

ivec2 IWidget::pos() const {
	return pos_;
}

ivec2 IWidget::size() const {
	return size_;
}

bool IWidget::hovered() const {
	return state_.hovered;
}

bool IWidget::pressed() const {
	return state_.pressed;
}

void IWidget::add_child(IWidget *child) {
	children_.push_back(child);
}
void IWidget::remove_child(IWidget *child) {
	std::erase(children_, child);
}

void IWidget::resize(ivec2 pos, ivec2 size) {
	pos_ = pos;
	size_ = size;
	_on_resize();
}

void IWidget::_on_resize() {
	if (parent_) {
		pos_.x = std::max(pos_.x, parent_->pos().x);
		pos_.y = std::max(pos_.y, parent_->pos().y);

		pos_.x = std::min(pos_.x, parent_->pos().x + parent_->size().x - size_.x);
		pos_.y = std::min(pos_.y, parent_->pos().y + parent_->size().y - size_.y);
	}
	on_resize();
	for (auto child : children_) {
		child->_on_resize();
	}
}
