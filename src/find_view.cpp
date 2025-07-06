#include "find_view.h"

#include "gp_shader.h"

using namespace glm;

FindView::HandleView::HandleView(Widget *parent) : Widget(parent) {}

void FindView::HandleView::update() {
}

void FindView::HandleView::draw() {
	GPShader::rect(pos(), size(), parent<FindView>()->color_, Z_UI_FG);
}

FindView::FindView(Widget *parent, std::function<void(FindView &)> &&on_find) : Widget(parent), on_find_(std::move(on_find)) {
	color_ = {0x00, 0xFF, 0x00, 0xFF}; // default color
	add_child(handle_);
	add_child(input_);
	add_child(but_prev_);
	add_child(but_next_);
	add_child(but_case_);
	add_child(but_word_);
	add_child(but_regex_);
}

bool FindView::on_key(int key, int scancode, int action, Window::KeyMods mods) {
	if (action != GLFW_PRESS) {
		return false;
	}

	if (key == GLFW_KEY_ENTER) {
		return true;
	}

	return false;
}

void FindView::handle_text() {
	if (on_find_) {
		on_find_(*this);
	}
}

FindView::Flags FindView::flags() const {
	return flags_;
}

color FindView::color() const {
	return color_;
}

std::string_view FindView::text() const {
	return input_.text();
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

void FindView::update() {
}

void FindView::draw() {
	// Scissor s{this};

	GPShader::rect(*this, pos(), {}, {0x80, 0x00, 0x00, 0xFF}, Z_UI_BG_1); // background
	handle_.draw();
	input_.draw();
	but_prev_.draw();
	but_next_.draw();
	but_case_.draw();
	but_word_.draw();
	but_regex_.draw();
}
