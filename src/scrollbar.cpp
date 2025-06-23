#include "scrollbar.h"

using namespace glm;

Scrollbar::Thumb::Thumb(bool horizontal, std::function<void(int)> &&scroll_cb)
	: Widget("T"), horizontal_(horizontal), scroll_cb_(std::move(scroll_cb)) {}

// bool Scrollbar::Thumb::on_mouse_button(ivec2 mouse, int button, int action, int mods) {
// 	return false;//hovered() && button == GLFW_MOUSE_BUTTON_LEFT;
// }

bool Scrollbar::Thumb::on_drag(ivec2 offset) {
	scroll_cb_(horizontal_ ? offset.x : offset.y);
	return true;
}

void Scrollbar::Thumb::draw() {
	Scissor s {this};
	GPShader::rect(pos(), size(), {100, hovered() * 255, pressed() * 255, 255}, Z_UI_FG);
}


Scrollbar::Scrollbar(bool horizontal, std::function<void(double)> &&scroll_cb)
	: Widget("S"), scroll_cb_(std::move(scroll_cb)), thumb_(horizontal, [this](int o){thumb_cb(o);}) {
	add_child(thumb_);
}

void Scrollbar::on_resize() {
	resize_thumb();
}

bool Scrollbar::on_cursor_pos(ivec2 mouse) {
	if (pressed(GLFW_MOUSE_BUTTON_LEFT)) {
		// scroll_toward_mouse(mouse, 1);
	} else if (pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		scroll_to_mouse(mouse);
	}
	return false;
}

/// If pressed below the thumb, move down one page. If pressed above the thumb, move up one page.
/// If right-clicked, move directly to the pressed position
bool Scrollbar::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	if (!hovered()) {
		return false;
	}
	// Ignore clicks on the thumb
	if (thumb_.hovered() && pressed(GLFW_MOUSE_BUTTON_LEFT)) {
		return true;
	}

	if (pressed(GLFW_MOUSE_BUTTON_LEFT)) {
		scroll_toward_mouse(mouse, 1);
	} else if (pressed(GLFW_MOUSE_BUTTON_RIGHT)) {
		scroll_to_mouse(mouse);
	}
	return true;
}

void Scrollbar::set(size_t position, size_t visible_extents, size_t total_extents) {
	position_percent_ = (double)position / (double)total_extents;
	visible_percent_ = (double)visible_extents / (double)total_extents;

	resize_thumb();
}

void Scrollbar::resize_thumb() {
	if (thumb_.horizontal_) {
		auto thumb_size = std::max<int>(50, (double)size().x * visible_percent_);
		double scroll_range = size().x - thumb_size;
		int thumb_top = scroll_range * position_percent_;
		thumb_.resize({pos().x + thumb_top, pos().y}, {thumb_size, size().y});
	} else {
		auto thumb_size = std::max<int>(50, (double)size().y * visible_percent_);
		double scroll_range = size().y - thumb_size;
		int thumb_top = scroll_range * position_percent_;
		thumb_.resize({pos().x, pos().y + thumb_top}, {size().x, thumb_size});
	}
}

size_t Scrollbar::scroll_range() const {
	if (thumb_.horizontal_) {
		return size().x - thumb_.size().x;
	}
	return size().y - thumb_.size().y;
}

void Scrollbar::scroll_toward_mouse(ivec2 mouse, uint pages) const {
	if (mouse.y < thumb_.pos().y) {
		scroll_cb_(std::max(0.0, position_percent_ - visible_percent_));
	} else {
		scroll_cb_(std::min(1.0, position_percent_ + visible_percent_));
	}
}

void Scrollbar::scroll_to_mouse(ivec2 mouse) const {
	int offset;
	if (thumb_.horizontal_) {
		offset = mouse.x - thumb_.pos().x - thumb_.size().x / 2; // Center the thumb on the mouse
	} else {
		offset = mouse.y - thumb_.pos().y - thumb_.size().y / 2; // Center the thumb on the mouse
	}
	thumb_cb(offset);
}

void Scrollbar::thumb_cb(int offset) const {
	auto scroll_percent = position_percent_ + offset / (double)scroll_range();
	scroll_cb_(scroll_percent);
}

void Scrollbar::draw() {
	Scissor s {this};
	if (visible_percent_ >= 1.0f) {
		return; // No need to draw the thumb if it covers the whole scrollbar
	}
	thumb_.draw();
	GPShader::rect(pos(), size(), {100, 100, 100, 50}, Z_UI_FG);
	GPShader::draw(); // TODO remove this, but scissoring messes it up since the scissor region is different by the time its drawn
}
