#include "content_view.h"
#include "file_view.h"

using namespace glm;

ContentView::ContentView(FileView &parent) : Widget("C"), parent_(parent) {
	add_child(&scroll_h_);
	add_child(&scroll_v_);
}

void ContentView::scroll_h_cb(double percent) {
	parent_.scroll_h_cb(percent);
}

void ContentView::scroll_v_cb(double percent) {
	parent_.scroll_v_cb(percent);
}

void ContentView::on_resize() {
	scroll_h_.resize({pos().x, pos().y + size().y - SCROLL_W}, {size().x - SCROLL_W, SCROLL_W});
	scroll_v_.resize({pos().x + size().x - SCROLL_W, pos().y}, {SCROLL_W, size().y});
	update_scrollbar();
}

void ContentView::update_scrollbar() {
	scroll_h_.set(parent_.scroll_.x, size().x, parent_.longest_line_ * TextShader::font().size.x);
	scroll_v_.set(parent_.scroll_.y, size().y, parent_.num_lines() * TextShader::font().size.y);
}

bool ContentView::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	if (pressed()) {
		// TODO if selection is in blank space, clip to the nearest left character
		ivec2 mouse_abs_char_loc = view_pos_to_abs_char_loc(rel_pos(mouse));
		selection_abs_char_loc = {mouse_abs_char_loc, mouse_abs_char_loc};
		selection_active_ = true;
	}
	return false;
}

bool ContentView::on_cursor_pos(ivec2 mouse) {
	if (!pressed()) {
		return false;
	}

	if (parent_.line_starts_.empty()) {
		return false;
	}

	// TODO if selection is in blank space, clip to the nearest left character
	ivec2 mouse_abs_char_loc = view_pos_to_abs_char_loc(rel_pos(mouse));
	selection_abs_char_loc.second = mouse_abs_char_loc;

	return false;
}

ivec2 ContentView::view_pos_to_abs_char_loc(ivec2 view_pos) const {
	return (view_pos + parent_.scroll_) / TextShader::font().size;
}

void ContentView::reset_mod_styles() {
	std::memcpy(mod_styles_.data(), base_styles_.data(), base_styles_.size() * sizeof(TextShader::CharStyle));
}

void ContentView::highlight_selection() {
	//// TODO highlight blank parts of selected lines
	// auto lo_buf_char_idx = parent_.abs_char_loc_to_buf_char_idx(selection_abs_char_loc.first);
	// auto hi_buf_char_idx = parent_.abs_char_loc_to_buf_char_idx(selection_abs_char_loc.second);
	//
	// if (hi_buf_char_idx < lo_buf_char_idx) {
	// 	std::swap(lo_buf_char_idx, hi_buf_char_idx);
	// }
	//
	// for (size_t buf_char_idx = lo_buf_char_idx; buf_char_idx < hi_buf_char_idx; buf_char_idx++) {
	// 	mod_styles_[buf_char_idx].bg = {200, 200, 200, 100};
	// }

	auto lo = selection_abs_char_loc.first;
	auto hi = selection_abs_char_loc.second;

	if (hi.y < lo.y || (hi.y == lo.y && hi.x < lo.x)) {
		std::swap(lo, hi);
	}

	for (auto y = lo.y; y <= hi.y; y++) {
		ivec2 pos, size;
		if (y == lo.y) {
			pos = {lo.x, y};
			if (y == hi.y) {
				size = {hi.x - lo.x, 1};
			} else {
				size = {this->size().x, 1};
			}
		} else if (y == hi.y) {
			if (lo.y == hi.y) {
				pos = {lo.x, y};
				size = {hi.x - lo.x, 1};
			} else {
				pos = {0, y};
				size = {hi.x, 1};
			}
		} else {
			pos = {0, y};
			size = {this->size().x, 1};
		}

		pos *= TextShader::font().size;
		pos += this->pos() - parent_.scroll_;
		size *= TextShader::font().size;

		GPShader::rect(pos, size, {200, 200, 200, 100}, 253);
	}
	GPShader::draw();
}

void ContentView::draw() {
	Scissor s {this};

	GPShader::rect(pos(), size(), {0x2B, 0x2B, 0x2B, 0xFF}, 254);
	GPShader::draw();
	// GPShader::rect({0, 0}, {200, 200}, {0, 255, 0, 100});
	// GPShader::rect({100, 100}, {200, 200}, {255, 0, 0, 100});

	TextShader::globals.frame_offset_px = pos();
	TextShader::globals.scroll_offset_px = parent_.scroll_;
	TextShader::globals.is_foreground = false;

	reset_mod_styles();

	if (selection_active_) {
		highlight_selection();
	}

	TextShader::use(buf_);

	glBindBuffer(GL_ARRAY_BUFFER, buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mod_styles_.size() * sizeof(TextShader::CharStyle), mod_styles_.data());

	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);

	TextShader::globals.is_foreground = true;
	TextShader::update_uniforms();
	parent_.draw_lines(render_range_);

	scroll_h_.draw();
	scroll_v_.draw();
}
