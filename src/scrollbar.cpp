#include "scrollbar.h"
#include "types.h"

using namespace glm;

Scrollbar::Thumb::Thumb(Widget *parent, std::function<void(int)> &&scroll_cb)
	: Widget(parent, "T"), scroll_cb_(std::move(scroll_cb)) {}

// bool Scrollbar::Thumb::on_mouse_button(ivec2 mouse, int button, int action, int mods) {
// 	return false;//hovered() && button == GLFW_MOUSE_BUTTON_LEFT;
// }

bool Scrollbar::Thumb::vert() {
	return parent<Scrollbar>()->vert_;
}

bool Scrollbar::Thumb::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	return button == GLFW_MOUSE_BUTTON_LEFT;
}

bool Scrollbar::Thumb::on_drag(ivec2 offset) {
	scroll_cb_(offset[vert()]);
	return true;
}

void Scrollbar::Thumb::update() {
}

void Scrollbar::Thumb::draw() {
	Scissor s {this};
	GPShader::rect(pos(), size(), {100, hovered() * 255, pressed() * 255, 255}, Z_UI_FG);
}


Scrollbar::Scrollbar(Widget *parent, bool vertical, std::function<void(double)> &&scroll_cb)
	: Widget(parent, "S"), scroll_cb_(std::move(scroll_cb)), thumb_(this, [this](int o){thumb_cb(o);}), vert_(vertical) {
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
	auto thumb_size = std::max<int>(50, (double)size()[vert_] * visible_percent_);
	double scroll_range = size()[vert_] - thumb_size;
	int thumb_top = scroll_range * position_percent_;

	if (vert_) {
		thumb_.resize({pos().x, pos().y + thumb_top}, {size().x, thumb_size});
	} else {
		thumb_.resize({pos().x + thumb_top, pos().y}, {thumb_size, size().y});
	}
}

size_t Scrollbar::scroll_range() const {
	return size()[vert_] - thumb_.size()[vert_];
}

void Scrollbar::scroll_toward_mouse(ivec2 mouse, uint pages) const {
	if (mouse[vert_] < thumb_.pos()[vert_]) {
		scroll_cb_(std::max(0.0, position_percent_ - visible_percent_ * pages));
	} else {
		scroll_cb_(std::min(1.0, position_percent_ + visible_percent_ * pages));
	}
}

void Scrollbar::scroll_to_mouse(ivec2 mouse) const {
	double percent = (double)(mouse[vert_] - pos()[vert_]) / (double)size()[vert_];
	scroll_cb_(std::clamp(percent, 0.0, 1.0));
}

void Scrollbar::thumb_cb(int offset) const {
	auto scroll_percent = position_percent_ + offset / (double)scroll_range();
	scroll_cb_(scroll_percent);
}

void Scrollbar::update() {
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
