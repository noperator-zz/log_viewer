#include "button_view.h"

#include "gp_shader.h"
#include "types.h"

using namespace glm;

ButtonView::ButtonView(Widget *parent, TexSet tex_set, const std::function<void()> &&on_click)
	: Widget(parent), tex_set_(tex_set), on_click_(std::move(on_click)) {}

void ButtonView::set_state(bool state) {
	state_ = state;
	soil();
}

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
		return true;
	}

	if (action != GLFW_RELEASE) {
		return false;
	}

	if (!hovered()) {
		// ignore the click if the mouse was released outside the button
		return true;
	}

	if (on_click_) {
		on_click_();
	}
	return true;
}

void ButtonView::update() {
}

void ButtonView::draw() {
	color bg = {0xFF, 0xFF, 0xFF, 0x00};
	TexID tex_id_ = tex_set_.idle;
	if (state_ || (hovered() && pressed())) {
		tex_id_ = tex_set_.active;
		bg.a = 0x80;
	}
	if (enabled_) {
		if (hovered() && !state_) {
			tex_id_ = tex_set_.hover;
			bg.a = 0x40;
		}
	} else {
		tex_id_ = tex_set_.disabled;
		bg = {0, 0, 0, 0x80}; // dimmed
	}
	GPShader::rect(*this, pos(), {}, bg, Z_UI_FG, tex_id_); // content
}

