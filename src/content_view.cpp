#include "content_view.h"
#include "file_view.h"
#include "util.h"

using namespace glm;

ContentView::ContentView(FileView &parent) : Widget("C"), parent_(parent) {
	add_child(scroll_h_);
	add_child(scroll_v_);
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
#if 1
	// TODO highlight blank parts of selected lines
	// FIXME messed up when mouse starts outside the text area
	auto lo_buf_char_idx = parent_.abs_char_loc_to_buf_char_idx(selection_abs_char_loc.first);
	auto hi_buf_char_idx = parent_.abs_char_loc_to_buf_char_idx(selection_abs_char_loc.second);

	if (hi_buf_char_idx < lo_buf_char_idx) {
		std::swap(lo_buf_char_idx, hi_buf_char_idx);
	}

	for (size_t buf_char_idx = lo_buf_char_idx; buf_char_idx < hi_buf_char_idx; buf_char_idx++) {
		mod_styles_[buf_char_idx].bg = {200, 200, 200, 100};
	}
#else
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

		GPShader::rect(pos, size, {200, 200, 200, 100}, Z_FILEVIEW_BG);
	}
#endif
}

void ContentView::highlight_findings() {
	auto lo_abs_char_idx = parent_.abs_char_loc_to_abs_char_idx(parent_.scroll_ / TextShader::font().size);
	auto hi_abs_char_idx = parent_.abs_char_loc_to_abs_char_idx((parent_.scroll_ + size()) / TextShader::font().size);

	// auto lo_buf_char_idx = parent_.abs_char_idx_to_buf_char_idx(lo_abs_char_idx);
	// auto hi_buf_char_idx = parent_.abs_char_loc_to_buf_char_idx((parent_.scroll_ + size()) / TextShader::font().size);

	// TODO Finder thread need to send a GLFW event when it finds a match, so a render is triggered
	std::lock_guard finder_lock(parent_.finder_.mutex());
	for (const auto &[ctx, job] : parent_.finder_.jobs()) {
		auto &find_view = *static_cast<const FindView *>(ctx);
		std::lock_guard job_lock(job->result_mtx());

		Timeit t("search");
		// binary search for the first result that starts at >= lo_buf_char_idx
		auto it = std::lower_bound(job->results_.begin(), job->results_.end(), lo_abs_char_idx,
		                           [](const auto &a, const auto &b) { return a.start < b; });
		t.stop();


		// iterate through all results that start at >= lo_buf_char_idx and <= hi_buf_char_idx
		for (; it != job->results_.end() && it->start <= hi_abs_char_idx; ++it) {
			auto start = parent_.abs_char_idx_to_buf_char_idx(it->start);
			auto end = parent_.abs_char_idx_to_buf_char_idx(it->end);

			// highlight the result
			for (size_t buf_char_idx = start; buf_char_idx < end; buf_char_idx++) {
				assert (buf_char_idx < mod_styles_.size());
				mod_styles_[buf_char_idx].bg = find_view.color();
			}
		}
	}
}

void ContentView::draw() {
	Scissor s {this};

	reset_mod_styles();

	GPShader::rect(pos(), size(), {0x2B, 0x2B, 0x2B, 0xFF}, Z_FILEVIEW_BG);
	if (selection_active_) {
		highlight_selection();
	}
	highlight_findings();
	GPShader::draw();

	TextShader::use(buf_);

	glBindBuffer(GL_ARRAY_BUFFER, buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, mod_styles_.size() * sizeof(TextShader::CharStyle), mod_styles_.data());

	TextShader::draw(pos(), parent_.scroll_, render_range_.x, render_range_.y - render_range_.x, Z_FILEVIEW_TEXT_FG, Z_FILEVIEW_TEXT_BG);

	scroll_h_.draw();
	scroll_v_.draw();
}
