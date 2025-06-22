#include "find_view.h"

#include "gp_shader.h"

using namespace glm;

FindView::HandleView::HandleView(const FindView &parent) : parent_(parent) {}

void FindView::HandleView::draw() {
	GPShader::rect(pos(), size(), parent_.color_, Z_UI_FG);
}

bool FindView::on_char(unsigned int codepoint, Window::KeyMods mods) {

	return false;
}

FindView::FindView(std::function<void(const FindView &)> on_find) {
	color_ = {0x00, 0xFF, 0x00, 0xFF}; // default color
	add_child(handle_);
	add_child(input_);
	add_child(but_prev_);
	add_child(but_next_);
	add_child(but_case_);
	add_child(but_word_);
	add_child(but_regex_);
}

FindView::Flags FindView::flags() const {
	return flags_;
}

color FindView::color() const {
	return color_;
}

void FindView::on_resize() {
	static constexpr int HANDLE_W = 20;
	static constexpr int BUTTON_W = 30;
	auto [x, y, w, h] = handle_.resize(pos(), {HANDLE_W, size().y});
	std::tie(x, y, w, h) = input_.resize({x+w, y}, {-BUTTON_W * 5 - w, h});
	std::tie(x, y, w, h) = but_prev_.resize({x+w, y}, {BUTTON_W, h});
	std::tie(x, y, w, h) = but_next_.resize({x+w, y}, {BUTTON_W, h});
	std::tie(x, y, w, h) = but_case_.resize({x+w, y}, {BUTTON_W, h});
	std::tie(x, y, w, h) = but_word_.resize({x+w, y}, {BUTTON_W, h});
	std::tie(x, y, w, h) = but_regex_.resize({x+w, y}, {BUTTON_W, h});
}

void FindView::draw() {
	// Scissor s{this};

	GPShader::rect(*this, pos(), {}, {0x80, 0x00, 0x00, 0xFF}, Z_UI_BG); // background
	handle_.draw();
	input_.draw();
	but_prev_.draw();
	but_next_.draw();
	but_case_.draw();
	but_word_.draw();
	but_regex_.draw();
}
