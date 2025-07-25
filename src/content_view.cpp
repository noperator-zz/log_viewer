#include "content_view.h"
#include "file_view.h"
#include "layout.h"
#include "TracyOpenGL.hpp"
#include "util.h"

using namespace glm;
using namespace std::chrono;

ContentView::ContentView(Widget *parent) : Widget(parent, "C") {
	add_child(scroll_h_);
	add_child(scroll_v_);
	add_child(stripe_view_);

	vlay_.add(nullptr, layout::Remain{100});
	vlay_.add(scroll_h_, SCROLL_W);

	hlay_.add(vlay_, layout::Remain{100});
	hlay_.add(scroll_v_, SCROLL_W);

	take_key_focus();
}

ContentView::~ContentView() {
}

FileView &ContentView::parent() { return *Widget::parent<FileView>(); }

void ContentView::scroll_to_cursor(bool center) {
	if (!center) {
		// TODO
		center = true;
	}

	auto px_loc = FileView::abs_char_loc_to_abs_px_loc(cursor_abs_char_loc_);
	auto pos = px_loc - size() / 2;

	parent().scroll_to(pos, false);
}

void ContentView::scroll_h_cb(double percent) {
	parent().scroll_to({percent * parent().max_scroll().x, parent().scroll_.y}, true);
}

void ContentView::scroll_v_cb(double percent) {
	parent().scroll_to({parent().scroll_.x, percent * parent().max_scroll().y}, true);
}

void ContentView::update_scrollbar() {
	// TODO the sizes are not quite right; they don't account for the overscroll region
	scroll_h_.set(parent().scroll_.x, size().x, parent().longest_line_ * TextShader::font().size.x);
	scroll_v_.set(parent().scroll_.y, size().y, parent().num_lines() * TextShader::font().size.y);
}

ivec2 ContentView::view_px_loc_to_abs_char_loc(ivec2 view_px_loc) {
	return (view_px_loc + parent().scroll_) / TextShader::font().size;
}

ivec2 ContentView::abs_px_loc_to_view_px_loc(ivec2 px_loc)  {
	return px_loc - parent().scroll_ + pos();
}

void ContentView::reset_mod_styles() {
	mod_styles_.resize_uninitialized(base_styles_.size());
	std::memcpy(mod_styles_.data(), base_styles_.data(), base_styles_.size() * sizeof(TextShader::CharStyle));
}

void ContentView::highlight_selection() {
#if 1
	// TODO highlight blank parts of selected lines
	// FIXME messed up when mouse starts outside the text area
	auto lo_buf_char_idx = parent().abs_char_loc_to_buf_char_idx(selection_abs_char_loc.first);
	auto hi_buf_char_idx = parent().abs_char_loc_to_buf_char_idx(selection_abs_char_loc.second);

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
		pos += this->pos() - parent().scroll_;
		size *= TextShader::font().size;

		GPShader::rect(pos, size, {200, 200, 200, 100}, Z_FILEVIEW_BG);
	}
#endif
}

void ContentView::highlight_findings(Finder::User &user) {
	auto lo_abs_char_idx = parent().abs_char_loc_to_abs_char_idx(parent().scroll_ / TextShader::font().size);
	auto hi_abs_char_idx = parent().abs_char_loc_to_abs_char_idx((parent().scroll_ + size()) / TextShader::font().size);

	// auto lo_buf_char_idx = parent().abs_char_idx_to_buf_char_idx(lo_abs_char_idx);
	// auto hi_buf_char_idx = parent().abs_char_loc_to_buf_char_idx((parent().scroll_ + size()) / TextShader::font().size);

	for (const auto &[ctx, job] : user.jobs()) {
		const auto find_view = static_cast<const FindView *>(ctx);
		const auto &results = job->results();

		// const auto &job = parent().finder_.jobs().at(&find_view);

		// Timeit t("search");
		// binary search for the first result that starts at >= lo_buf_char_idx
		auto it = std::lower_bound(results.begin(), results.end(), lo_abs_char_idx,
		                           [](const auto &a, const auto &b) { return a.start < b; });
		// t.stop();


		// iterate through all results that start at >= lo_buf_char_idx and <= hi_buf_char_idx
		for (; it != results.end() && it->start <= hi_abs_char_idx; ++it) {
			auto start = parent().abs_char_idx_to_buf_char_idx(it->start);
			auto end = parent().abs_char_idx_to_buf_char_idx(it->end);

			// highlight the result
			for (size_t buf_char_idx = start; buf_char_idx < end; buf_char_idx++) {
				assert (buf_char_idx < mod_styles_.size());
				mod_styles_[buf_char_idx].bg = find_view->color();
			}
		}
	}
}

bool ContentView::on_mouse_button(ivec2 mouse, int button, int action, Window::KeyMods mods) {
	if (pressed()) {
		// TODO if selection is in blank space, clip to the nearest left character
		cursor_abs_char_loc_ = view_px_loc_to_abs_char_loc(rel_pos(mouse));
		selection_abs_char_loc = {cursor_abs_char_loc_, cursor_abs_char_loc_};
		selection_active_ = true;
	}
	soil();
	return true;
}

bool ContentView::on_cursor_pos(ivec2 mouse) {
	if (!pressed()) {
		return false;
	}

	if (parent().line_starts_.empty()) {
		return false;
	}

	// TODO if selection is in blank space, clip to the nearest left character
	cursor_abs_char_loc_ = view_px_loc_to_abs_char_loc(rel_pos(mouse));
	selection_abs_char_loc.second = cursor_abs_char_loc_;

	soil();
	return true;
}

void ContentView::on_resize() {
	stripe_view_.resize({pos().x + size().x - 30, pos().y}, {30, size().y});

	hlay_.apply(*this);
	update_scrollbar();
}

void ContentView::update_from_parent(Finder::User &finder_user) {
	ZoneScopedN("ContentView update");
	reset_mod_styles();

	if (selection_active_) {
		highlight_selection();
	}
	highlight_findings(finder_user);

	{
		ZoneScopedN("ContentView buffer");
		// TracyGpuZone("ContentView buffer");
		TextShader::use(buf_);
		glBindBuffer(GL_ARRAY_BUFFER, buf_.vbo_style);
		glBufferSubData(GL_ARRAY_BUFFER, 0, mod_styles_.size() * sizeof(TextShader::CharStyle), mod_styles_.data());
	}
}

void ContentView::update() {}

static bool cursor_visible() {
	// 500ms blink
	auto now = steady_clock::now();
	return duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000 < 500;
}

void ContentView::draw() {
	// TracyGpuZone("ContentView draw");
	GPShader::rect(pos(), size(), {0x2B, 0x2B, 0x2B, 0xFF}, Z_FILEVIEW_BG);
	GPShader::draw();

	TextShader::use(buf_);
	TextShader::draw(pos(), parent().scroll_, 0, mod_styles_.size(), Z_FILEVIEW_TEXT_FG);

	if (cursor_visible()) {
		auto view_px_loc = abs_px_loc_to_view_px_loc(FileView::abs_char_loc_to_abs_px_loc(cursor_abs_char_loc_));
		GPShader::rect(view_px_loc, ivec2{2, TextShader::font().size.y}, {0xFF, 0xFF, 0xFF, 0xFF}, Z_UI_FG);
		Scissor s {this}; // only needed for the cursor
		GPShader::draw();
	}

	stripe_view_.draw();

	scroll_h_.draw();
	scroll_v_.draw();
}
