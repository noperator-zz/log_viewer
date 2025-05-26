#include "widget.h"

#include <GLFW/glfw3.h>


IWidget::IWidget(glm::uvec2 pos, glm::uvec2 size) : pos_(pos), size_(size) {
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

bool IWidget::handle_cursor_pos(glm::uvec2 pos) {
	for (auto child : children_) {
		child->handle_cursor_pos(pos);
	}

	if (pos.x >= pos_.x && pos.x < pos_.x + size_.x &&
	    pos.y >= pos_.y && pos.y < pos_.y + size_.y) {
		if (!state_.hovered) {
			state_.hovered = true;
			on_hover();
		}
		on_cursor_pos(pos);
	} else {
		if (state_.hovered) {
			state_.hovered = false;
			on_unhover();
		}
	}
	return state_.hovered; // Return true if hovered, false otherwise
}

glm::uvec2 IWidget::pos() const {
	return pos_;
}

glm::uvec2 IWidget::size() const {
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

void IWidget::resize(glm::uvec2 pos, glm::uvec2 size) {
	pos_ = pos;
	size_ = size;
	on_resize();
}
