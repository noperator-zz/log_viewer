#include "input_view.h"

#include "gp_shader.h"

using namespace glm;

InputView::InputView(Widget *parent, const std::function<void(std::string_view)> &&on_update)
	: Widget(parent), on_update_(std::move(on_update)) {
	TextShader::create_buffers(buf_, BUFFER_SIZE);
}

bool InputView::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods)  {
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		assert(hovered());
		take_key_focus();

		soil();
		return true;
	}
	return false;
}

bool InputView::on_key(int key, int scancode, int action, Window::KeyMods mods) {
	if (action == GLFW_RELEASE) {
		return false;
	}

	if (key == GLFW_KEY_LEFT) {
		if (cursor_ > 0) {
			cursor_--;
		}
		soil();
		return true;
	}

	if (key == GLFW_KEY_RIGHT) {
		if (cursor_ < text_.size()) {
			cursor_++;
		}
		soil();
		return true;
	}

	if (key == GLFW_KEY_BACKSPACE) {
		if (!text_.empty() && cursor_ > 0) {
			text_.erase(cursor_ - 1, 1);
			cursor_--;
			if (on_update_) {
				on_update_(text_);
			}
		}
		soil();
		return true;
	}

	if (key == GLFW_KEY_DELETE) {
		if (!text_.empty() && cursor_ < text_.size()) {
			text_.erase(cursor_, 1);
			if (on_update_) {
				on_update_(text_);
			}
		}
		soil();
		return true;
	}

	if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && mods.control) {
		text_.insert(cursor_, "\n");
		cursor_++;
		if (on_update_) {
			on_update_(text_);
		}
		soil();
		return true;
	}
	return false;
}

bool InputView::on_char(unsigned int codepoint, Window::KeyMods mods) {
	if (mods.special()) {
		return false;
	}
	text_.insert(cursor_, 1, static_cast<char>(codepoint));
	cursor_++;
	if (on_update_) {
		on_update_(text_);
	}
	soil();
	return true;
}

std::string_view InputView::text() const {
	return text_;
}

void InputView::on_resize() {

}

void InputView::update() {
}

void InputView::draw() {
	auto [x, y, w, h] = GPShader::rect(*this, pos() + ivec2{2}, ivec2{-4}, {}, Z_UI_BG_1); // transparent text box inside
	GPShader::rect(*this, pos(), {}, {0xFF, 0xFF, 0xFF, 0xFF}, Z_UI_BG_2); // text box border
	// Since inside is transparent, must DPShader draw() before text so that the background (from parent) is drawn first
	//  and text is correctly blended over it


	GPShader::draw();

	std::vector<ivec2> coords {};
	TextShader::use(buf_);
	TextShader::render(buf_, text_, {{}, {}, {0xFF, 0xFF, 0xFF, 0xFF}, {}}, {cursor_}, coords);
	TextShader::draw({x, y}, {}, 0, text_.size(), Z_UI_FG);

	GPShader::rect(ivec2{x, y} + coords[0] * TextShader::font().size, ivec2{2, TextShader::font().size.y}, {0xFF, 0xFF, 0xFF, 0xFF}, Z_UI_FG); // cursor
}

