#include "input_view.h"

#include "gp_shader.h"

using namespace glm;

InputView::InputView(const std::function<void(std::string_view)> &&on_update) : on_update_(on_update) {
	TextShader::create_buffers(buf_, BUFFER_SIZE);
}

bool InputView::on_key(int key, int scancode, int action, Window::KeyMods mods) {
	if (action != GLFW_PRESS) {
		return false;
	}

	if (scancode == GLFW_KEY_BACKSPACE) {
		if (!text_.empty()) {
			text_.pop_back();
			if (on_update_) {
				on_update_(text_);
			}
		}
		return true;
	}

	if (scancode == GLFW_KEY_ENTER && mods.control) {
		text_.push_back('\n');
		if (on_update_) {
			on_update_(text_);
		}
		return true;
	}
	return false;
}

bool InputView::on_char(unsigned int codepoint, Window::KeyMods mods) {
	if (mods.special()) {
		return false;
	}
	text_.push_back(codepoint);
	if (on_update_) {
		on_update_(text_);
	}
	return true;
}

void InputView::on_resize() {

}

void InputView::draw() {
	GPShader::rect(*this, pos() + ivec2{2}, ivec2{-4}, {}, Z_UI_FG); // transparent text box inside
	GPShader::rect(*this, pos(), {}, {0xFF, 0xFF, 0xFF, 0xFF}, Z_UI_FG); // text box border

	TextShader::use(buf_);
	TextShader::render(buf_, text_, {{}, {}, {0xFF, 0xFF, 0xFF, 0xFF}, {}});
	TextShader::draw(pos(), {}, 0, text_.size(), Z_UI_FG, Z_UI_BG);
}

