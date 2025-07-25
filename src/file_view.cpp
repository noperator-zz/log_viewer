#include <iostream>

#include "file_view.h"

#include <mutex>

#include "color.h"
#include "log.h"
#include "settings.h"
#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;
using namespace std::chrono;

std::unique_ptr<FileView> FileView::create(Widget *parent, const char *path) {
	return std::unique_ptr<FileView>(new FileView(parent, path));
}

FileView::FileView(Widget *parent, const char *path)
	: Widget(parent, "FV"), loader_(File{path}, dataset_, [this]{on_new_lines();}) {

	add_child(linenum_view_);
	add_child(content_view_);
}

FileView::~FileView() {
	loader_.stop();
	finder_.stop();
	TextShader::destroy_buffers(content_view_.buf_);
	TextShader::destroy_buffers(linenum_view_.buf_);
}

int FileView::open() {
	int err = loader_.start();
	if (err != 0) {
		return err;
	}

	TextShader::create_buffers(content_view_.buf_, CONTENT_BUFFER_SIZE);
	TextShader::create_buffers(linenum_view_.buf_, LINENUM_BUFFER_SIZE);

	return 0;
}

void FileView::on_new_lines() {
	// content_view_.soil();
	soil();
	// Window::send_event();
}

bool FileView::on_key(int key, int scancode, int action, Window::KeyMods mods) {
	if (mods.control && key == GLFW_KEY_F && action == GLFW_PRESS) {

		color color = UNIQUE_COLORS[0];
		for (::color c : UNIQUE_COLORS) {
			if (std::none_of(find_views_.begin(), find_views_.end(),
				[&c](const auto &view) { return view->color() == c; })) {
				color = c;
				break;
			}
		}

		find_views_.emplace_back(std::make_unique<FindView>(this, color,
			[this](auto &find_view, auto event) { content_view_.on_findview_event(find_view, event); }
		));
		add_child(*find_views_.back().get());
		on_resize();
		return true;
	}
	return false;
}

void FileView::scroll_to(ivec2 pos, bool allow_autoscroll) {
	auto max = max_scroll();
	scroll_ = pos;
	autoscroll_ = allow_autoscroll && scroll_.y >= max_visible_scroll().y;

	scroll_.x = std::clamp(scroll_.x, 0, max.x);
	scroll_.y = std::clamp(scroll_.y, 0, max.y);
	content_view_.update_scrollbar();
	soil();
}

void FileView::scroll(dvec2 scroll) {
	auto max = max_scroll();

	scroll *= TextShader::font().size;
	scroll *= -3;
	if (scroll.y < 0) {
		autoscroll_ = false;
	}

	auto frac = scroll - dvec2(ivec2(scroll));
	frac_scroll_ += frac;
	auto mant = ivec2(frac_scroll_);
	frac_scroll_ -= dvec2(mant);

	scroll_ += mant + ivec2(scroll);
	if (scroll_.y >= max_visible_scroll().y) {
		autoscroll_ = true;
	}

	scroll_.x = std::clamp(scroll_.x, 0, max.x);
	scroll_.y = std::clamp(scroll_.y, 0, max.y);

	content_view_.update_scrollbar();
	soil();
}

static size_t linenum_len(size_t line) {
	return line == 0 ? 1 : (size_t)log10(line) + 1;
}

size_t FileView::get_line_len(size_t line_idx) const {
	assert(line_idx < line_starts_.size() - 1);
	return line_starts_[line_idx + 1] - line_starts_[line_idx];
}

size_t FileView::num_lines() const {
	if (line_starts_.empty()) {
		return 0;
	}
	// last entry in line_starts_ is the end of the file (even if it's not a newline character), so we don't count it
	assert(line_starts_.size() >= 2);
	return line_starts_.size() - 1;
}

ivec2 FileView::max_scroll() const {
	return TextShader::font().size * ivec2{longest_line_, num_lines()};
}

ivec2 FileView::max_visible_scroll() const {
	auto max = max_scroll() - content_view_.size();
	max.x = std::max(max.x, 0);
	max.y = std::max(max.y, 0);
	return max;
}

// NOTE Terminology:
//  - abs_pos:         Pixel coordinate (x, y) relative to the window
//  - view_pos:        Pixel coordinate (x, y) relative to the widget position
//  - abs_char_loc:    Character coordinate (col, row/line) relative to the start of the file
//  - buf_char_loc:    Character coordinate (col, row/line) relative to the start of the GPU buffer
//  - abs_char_idx:    Character index relative to the start of the file
//  - buf_char_idx:    Character index relative to the start of the GPU buffer
//  - buf:             The subset of lines currently loaded in the GPU buffer, which is larger than the visible range by ~2 * OVERSCAN_LINES

// NOTE Trivial Conversions:
//  abs_pos -> view_pos: rel_pos(abs_pos)
//  -
//  abs_char_loc -> abs_char_idx: line_starts[abs_loc.y] + abs_loc.x
//  abs_char_loc -> buf_char_loc: abs_loc - {buf_lines_.x, 0}
//  -
//  abs_char_idx -> buf_char_idx: abs_idx - line_starts[buf_lines_.x]
//  buf_char_loc -> buf_char_idx: abs_char_loc(buf_loc) - line_start[buf_lines_.x]
//  -

ivec2 FileView::abs_char_loc_to_abs_px_loc(ivec2 abs_loc) {
	return abs_loc * TextShader::font().size;
}

size_t FileView::abs_char_loc_to_abs_char_idx(ivec2 abs_loc) const {
	if (abs_loc.y < 0) {
		return 0;
	}
	abs_loc.y = std::min(abs_loc.y, (int)line_starts_.size() - 1);
	return line_starts_[abs_loc.y] + abs_loc.x;
}

size_t FileView::abs_char_idx_to_buf_char_idx(size_t abs_idx) const {
	if (abs_idx < line_starts_[content_view_.buf_char_window_.tl.y]) {
		return 0; // If the index is before the start of the buffer, return 0
	}
	if (abs_idx > line_starts_[content_view_.buf_char_window_.br.y]) {
		abs_idx = line_starts_[content_view_.buf_char_window_.br.y];
	}
	return abs_idx - line_starts_[content_view_.buf_char_window_.tl.y];
}

size_t FileView::abs_char_loc_to_buf_char_idx(ivec2 abs_loc) const {
	return abs_char_idx_to_buf_char_idx(abs_char_loc_to_abs_char_idx(abs_loc));
}

void FileView::really_update_buffers(const uint8_t *data) {
	Timeit update_timeit("Update buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	assert(num_lines() > 0);

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	// TODO detect if content won't fit in the buffer
	// TODO Use glMapBufferRange
	size_t content_num_chars = 0;
	size_t linenum_num_chars = 0;
	char linenum_text[linenum_view_.linenum_chars_ + 1]; // +1 for the null terminator added by sprintf
	const std::string fmt = "%" + std::to_string(linenum_view_.linenum_chars_) + "zu";

	for (int line_idx = std::max(0, content_view_.buf_char_window_.tl.y); line_idx < std::clamp(content_view_.buf_char_window_.br.y, 0, (int)num_lines()); line_idx++) {
		int line_len = get_line_len(line_idx);
		size_t linenum_len = sprintf(linenum_text, fmt.c_str(), line_idx + 1);

		for (int char_idx = std::max(0, content_view_.buf_char_window_.tl.x); char_idx < std::clamp(content_view_.buf_char_window_.br.x, 0, line_len); char_idx++) {
			uint8_t r = 200;
			uint8_t g = 200;
			uint8_t b = 200;
			content_view_.base_styles_[content_num_chars] = {
				uvec2{char_idx, line_idx},
				vec4{r, g, b, 255},
				// vec4{0, 0, 0, 255}
				// vec4{100, 0, 0, 255}
				// vec4{255, 255, 255, 255}
				// vec4{100, 0, 0, 100}
				vec4{},
				{},
				data[line_starts_[line_idx] + char_idx],
			};
			content_num_chars++;
		}

		for (uint i = 0; i < linenum_len; i++) {
			linenum_view_.styles_[linenum_num_chars] = {
				uvec2{i, line_idx},
				vec4{0xA0, 0xA0, 0xA0, 255},
				vec4{},
				{},
				(uint8_t)linenum_text[i],
			};
			linenum_num_chars++;
		}
	}
	content_view_.base_styles_.resize_uninitialized(content_num_chars);
	linenum_view_.styles_.resize_uninitialized(linenum_num_chars);

	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, linenum_view_.styles_.size() * sizeof(TextShader::CharStyle), linenum_view_.styles_.data());

	// update_timeit.stop();
	// printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	// printf("Linenum buffer size: %zu\n", total_linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	logger << content_view_.buf_char_window_.tl.x << " " << content_view_.buf_char_window_.tl.y
	       << " " << content_view_.buf_char_window_.br.x << " " << content_view_.buf_char_window_.br.y
	       << " Content chars: " << content_num_chars
	       << " Linenum chars: " << linenum_num_chars
	       << std::endl;
 	content_view_.soil();
	linenum_view_.soil();
}

void FileView::update_buffers(const Dataset::User &user) {
	if (num_lines() == 0) {
		return;
	}

	// first and last (inclusive) rows and cols which would be visible on the screen
	rect<int> visible_char_window = {
		scroll_ / TextShader::font().size,
		((scroll_ + content_view_.size()) / TextShader::font().size)
	};

	// This will fail if the screen is larger than the buffer
	assert((visible_char_window.size().x + 1) <= MAX_VISIBLE_CHARS.x);
	assert((visible_char_window.size().y + 1) <= MAX_VISIBLE_CHARS.y);

	// TODO also run this when new lines are added and they're visible
	// Check if the visible lines are already in the buffer
	// if (!content_view_.buf_char_window_.contains(visible_char_window)) {
	if (1) {
		content_view_.buf_char_window_ = content_view_.buf_char_window_.from_center_size(
			visible_char_window.center(),
			MAX_VISIBLE_CHARS
		);
		really_update_buffers(user.data());
	}

	// size_t abs_buf_start_char = line_starts_[content_view_.buf_char_window_.tl.y] + content_view_.buf_char_window_.tl.x;
	// size_t abs_visible_start_char = line_starts_[visible_char_window.tl.y] + visible_char_window.tl.x;
	// content_render_range = u64vec2{abs_visible_start_char, abs_visible_start_char + buf_num_chars_} - abs_buf_start_char;
	//
	// linenum_render_range = (uvec2{visible_char_window.tl.y, visible_char_window.br.y} - (uint)content_view_.buf_char_window_.tl.y) * (uint)linenum_chars_;
}

void FileView::on_resize() {
	// TODO cache the layout; update only when linenum_chars_ or font size changes
	layout::H hlay {};
	layout::V vlay {};

	for (auto &view : find_views_) {
		vlay.add(view.get(), 36);
	}

	hlay.add(linenum_view_, linenum_view_.linenum_chars_ * TextShader::font().size.x + 20);
	hlay.add(content_view_, layout::Remain{100});
	vlay.add(hlay, layout::Remain{100});
	vlay.apply(*this);
}


void FileView::update() {
	static microseconds duration {};
	// Timeit timeit("FileView::update");

	auto now = steady_clock::now();
	loader_.get(line_starts_, longest_line_);

	if (line_starts_.empty()) {
		return;
	}

	auto prev_linenum_chars = linenum_view_.linenum_chars_;

	if (autoscroll_) {
		// jump to the end of the file
		scroll_.y = std::max(scroll_.y, max_visible_scroll().y);
	}

	linenum_view_.linenum_chars_ = linenum_len(num_lines());

	{
		auto user = dataset_.user();
		update_buffers(user);
	}

	if (linenum_view_.linenum_chars_ != prev_linenum_chars) {
		on_resize();
	}
	// TODO this is only needed if longest_line_ or num_lines_ changed
	content_view_.update_scrollbar();

	// fflush(stdout);
	// Window::send_event();

	duration += duration_cast<microseconds>(steady_clock::now() - now);
	// std::cout << "FileView::update total duration: " << duration.count() << "us\n";

	// TODO temporary
	content_view_.soil();
	linenum_view_.soil();
}

void FileView::draw() {
	// std::cout << content_render_range.x << " " << content_render_range.y << "\n";

	// TextShader::globals.frame_offset_px = pos();

	// Timeit draw("Draw");
	//
	for (auto &view : find_views_) {
		view->draw();
	}

	content_view_.draw();
	linenum_view_.draw();

	// draw.stop();
}
