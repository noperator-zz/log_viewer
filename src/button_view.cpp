#include "button_view.h"

#include "gp_shader.h"
#include "types.h"

using namespace glm;

ButtonView::ButtonView(Widget *parent, TexID tex_id, bool toggle, const std::function<void(bool)> &&on_click)
	: Widget(parent), tex_id_(tex_id), toggle_(toggle), on_click_(std::move(on_click)) {}

void ButtonView::set_enabled(bool enabled) {
	enabled_ = enabled;
	soil();
}

void ButtonView::on_hover(bool state) {
	if (enabled_) {
		soil();
	}
}

bool ButtonView::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	if (!enabled_) {
		return false;
	}

	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return false;
	}

	if (action == GLFW_PRESS) {
		state_ = !state_;
		return true;
	}

	if (action != GLFW_RELEASE) {
		return false;
	}

	if (!hovered()) {
		// undo the state change if the button was released outside the button
		state_ = !state_;
		return true;
	}

	if (!toggle_) {
		// if not toggle, reset state to false on release
		state_ = false;
	}

	if (on_click_) {
		on_click_(state_);
	}
	return true;
}

void ButtonView::update() {
}

void ButtonView::draw() {
	GPShader::rect(*this, pos(), {}, {0xFF, 0xFF, 0x00, 0xFF}, Z_UI_FG); // border
	auto [x, y, w, h] = GPShader::rect(*this, pos() + ivec2{2}, ivec2{-4}, {0x00, 0x00, 0x80, 0xFF}, Z_UI_FG, tex_id_); // content
	if (state_) {
		GPShader::rect(*this, {x, y}, {w, h}, {0xFF, 0xFF, 0xFF, 0x40}, Z_UI_FG); // highlight
	}
	if (enabled_) {
		if (hovered()) {
			GPShader::rect(*this, {x, y}, {w, h}, {0xFF, 0xFF, 0xFF, 0x40}, Z_UI_FG); // highlight
		}
	} else {
		GPShader::rect(*this, {x, y}, {w, h}, {0x00, 0x00, 0x00, 0x40}, Z_UI_FG); // disabled
	}
}

