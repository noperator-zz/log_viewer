#include "button_view.h"

#include "gp_shader.h"

using namespace glm;

ButtonView::ButtonView(const std::function<void(bool)> &&on_click) : on_click_(on_click) {}

bool ButtonView::on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) {
	if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_RELEASE) {
		return false;
	}
	state_ = !state_;
	if (on_click_) {
		on_click_(state_);
	}
	return true;
}

void ButtonView::draw() {
	GPShader::rect(*this, pos(), {}, {0xFF, 0xFF, 0x00, 0xFF}, Z_UI_FG);
	GPShader::rect(*this, pos() + ivec2{2}, ivec2{-4}, {0x00, 0x00, 0x80, 0xFF}, Z_UI_FG-1);
}

