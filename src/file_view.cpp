#include <iostream>

#include "file_view.h"

#include <mutex>

#include "../shaders/text_shader.h"
#include "util.h"

using namespace glm;
using namespace std::chrono;

std::unique_ptr<FileView> FileView::create(const char *path) {
	return std::unique_ptr<FileView>(new FileView(path));
}

FileView::FileView(const char *path)
	: Widget("FV"), loader_(File{path}, dataset_, [this]{on_new_lines();}) {

	add_child(linenum_view_);
	add_child(content_view_);
	add_child(stripe_view_);

	find_ctxs_.emplace_back(std::make_unique<FindContext>([this](auto &find_view) { handle_findview(find_view); }));
	add_child(find_ctxs_.back()->view_);
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

void FileView::handle_findview(FindView &find_view) {
	int ret = finder_.submit(&find_view, [this](auto ctx, auto idx){on_find(ctx, idx);}, find_view.text(), 0);//find_view.flags());
	if (ret != 0) {
		std::cerr << "Error submitting find request: " << ret << std::endl;
	}

	stripe_view_.remove_dataset(&find_view);
	stripe_view_.add_dataset(&find_view, find_view.color(), [](const void *ele) {
		return reinterpret_cast<const Finder::Job::Result*>(ele)->start;
	});
}

void FileView::on_find(void *ctx, size_t idx) {
	// TODO dedicated find_ctx mutex
	// std::lock_guard lock(line_mtx_);
	// static_cast<FindContext *>(ctx)->next_report_ = idx;
	soil();
}

void FileView::on_new_lines() {
	soil();
	// Window::send_event();
}

void FileView::scroll_h_cb(double percent) {
	auto max = max_scroll();

	scroll_.x = percent * max.x;
	scroll_.x = std::clamp(scroll_.x, 0, max.x);
	content_view_.update_scrollbar();
}

void FileView::scroll_v_cb(double percent) {
	auto max = max_scroll();

	scroll_.y = percent * max.y;
	autoscroll_ = scroll_.y >= max_visible_scroll().y;

	scroll_.y = std::clamp(scroll_.y, 0, max.y);
	content_view_.update_scrollbar();
}

void FileView::scroll(ivec2 scroll) {
	auto max = max_scroll();

	scroll *= TextShader::font().size;
	scroll *= -3;
	if (scroll.y < 0) {
		autoscroll_ = false;
	}

	scroll_ += scroll;
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
	return line_starts_.empty() ? 0 : line_starts_.size() - 1;
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

size_t FileView::abs_char_loc_to_abs_char_idx(ivec2 abs_loc) const {
	if (abs_loc.y < 0) {
		return 0;
	}
	abs_loc.y = std::min(abs_loc.y, (int)line_starts_.size() - 1);
	return line_starts_[abs_loc.y] + abs_loc.x;
}

size_t FileView::abs_char_idx_to_buf_char_idx(size_t abs_idx) const {
	if (abs_idx < line_starts_[buf_lines_.x]) {
		return 0; // If the index is before the start of the buffer, return 0
	}
	if (abs_idx > line_starts_[buf_lines_.y]) {
		abs_idx = line_starts_[buf_lines_.y];
	}
	return abs_idx - line_starts_[buf_lines_.x];
}

size_t FileView::abs_char_loc_to_buf_char_idx(ivec2 abs_loc) const {
	return abs_char_idx_to_buf_char_idx(abs_char_loc_to_abs_char_idx(abs_loc));
}

void FileView::really_update_buffers(int start, int end, const uint8_t *data) {
	// Timeit update_timeit("Update content buffer");

	// first (inclusive) and last (exclusive) lines to buffer
	buf_lines_ = {start, end};
	assert(buf_lines_.y > 0);
	assert(num_lines() > 0);

	// TODO The lines on screen have already been parsed. Cache and reuse them instead of re-parsing

	const auto first_line_start = line_starts_[buf_lines_.x];

	// TODO detect if content won't fit in the buffer
	// TODO If there is an extremely long line, only buffer the visible portion of the line
	// TODO Handle 0 mapped lines, and other cases
	// TODO Use glMapBufferRange
	glBindBuffer(GL_ARRAY_BUFFER, content_view_.buf_.vbo_text);
	size_t num_chars = 0;
	num_chars = line_starts_[buf_lines_.y] - first_line_start;
	glBufferSubData(GL_ARRAY_BUFFER, 0, num_chars, data + first_line_start);

	content_view_.base_styles_.resize_uninitialized(num_chars);
	content_view_.mod_styles_.resize_uninitialized(num_chars);
	size_t c = 0;
	size_t current_char = first_line_start;

	// auto next_match = matches.begin();

	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// while (next_match != matches.end() && current_char > next_match->end) {
		// 	// Skip matches that are before the current character
		// 	next_match++;
		// }

		size_t line_len = get_line_len(line_idx);
		for (size_t i = 0; i < line_len; i++) {
			bool is_match = false;//next_match != matches.end() && current_char >= next_match->start && current_char < next_match->end;
			uint8_t r = is_match ? 100 : 200;
			uint8_t g = 200;
			uint8_t b = 200;
			content_view_.base_styles_[c] = {
				{},
				uvec2{i, line_idx},
				vec4{r, g, b, 255},
				vec4{}
				// vec4{0, 0, 0, 255}
				// vec4{100, 0, 0, 255}
				// vec4{255, 255, 255, 255}
				// vec4{100, 0, 0, 100}
			};
			c++;
			current_char++;
		}
	}

	const auto num_lines = buf_lines_.y - buf_lines_.x;
	// max number of characters for all line numbers
	const auto total_linenum_chars = linenum_chars_ * num_lines;

	auto linenum_text = std::make_unique<uint8_t[]>(total_linenum_chars + 1); // +1 for the null terminator added by sprintf
	auto linenum_styles = std::make_unique<TextShader::CharStyle[]>(total_linenum_chars);

	c = 0;

	const std::string fmt = "%" + std::to_string(linenum_chars_) + "zu";
	for (uint line_idx = buf_lines_.x; line_idx < buf_lines_.y; line_idx++) {
		// std::string linenum_str = std::to_string(line_idx + 1);
		// linenum_text.insert(linenum_text.end(), linenum_str.begin(), linenum_str.end());
		size_t line_len = sprintf((char*)&linenum_text[c], fmt.c_str(), line_idx + 1);

		for (size_t i = 0; i < line_len; i++) {
			linenum_styles[c] = {
				{},
				uvec2{i, line_idx},
				vec4{0xA0, 0xA0, 0xA0, 255},
				vec4{}
			};
			c++;
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_text);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars, linenum_text.get());

	glBindBuffer(GL_ARRAY_BUFFER, linenum_view_.buf_.vbo_style);
	glBufferSubData(GL_ARRAY_BUFFER, 0, total_linenum_chars * sizeof(TextShader::CharStyle), linenum_styles.get());

	// update_timeit.stop();
	// printf("Content buffer size: %zu\n", num_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
	// printf("Linenum buffer size: %zu\n", total_linenum_chars * (sizeof(TextShader::CharStyle) + sizeof(uint8_t)));
}

void FileView::update_buffers(uvec2 &content_render_range, uvec2 &linenum_render_range, const Dataset::User &user) {
	content_render_range = {};
	linenum_render_range = {};

	if (num_lines() == 0) {
		return;
	}

	// first (inclusive) and last (exclusive) lines which would be visible on the screen
	ivec2 visible_lines = ivec2{0, 1} + ivec2{scroll_.y, scroll_.y + size().y} / TextShader::font().size.y;

	// Clamp to the total number of lines available
	visible_lines.x = std::min(visible_lines.x, (int)num_lines() - 1);
	visible_lines.y = std::min(visible_lines.y, (int)num_lines());

	// Check if the visible lines are already in the buffer
	if (visible_lines.x < buf_lines_.x || visible_lines.y > buf_lines_.y) {
		really_update_buffers(
			// first (inclusive) and last (exclusive) lines to buffer
			std::max(0LL, visible_lines.x - OVERSCAN_LINES),
			std::min((ssize_t)num_lines(), visible_lines.y + OVERSCAN_LINES),
			user.data()
		);
	}

	auto buf_start_char = line_starts_[buf_lines_.x];
	content_render_range = u64vec2{
		line_starts_[visible_lines.x],
		line_starts_[visible_lines.y]
	} - buf_start_char;

	linenum_render_range = (uvec2{visible_lines.x, visible_lines.y} - (uint)buf_lines_.x) * (uint)linenum_chars_;
}

void FileView::on_resize() {
	auto [x, y, w, h] = linenum_view_.resize(pos(), {linenum_chars_ * TextShader::font().size.x + 20, size().y});
	std::tie(x, y, w, h) = content_view_.resize({x + w, y}, {size().x - w, h});
	stripe_view_.resize({x + w - 30, y}, {30, h});
	for (auto &ctx : find_ctxs_) {
		ctx->view_.resize({x, y}, {content_view_.size().x - content_view_.scroll_v_.size().x, 30});
		y += 30;
	}
}


void FileView::update() {
	static microseconds duration {};
	// Timeit timeit("FileView::update");

	auto now = steady_clock::now();
	Loader::State loader_state;
	loader_.get(line_starts_, longest_line_, loader_state);

	if (line_starts_.empty()) {
		return;
	}

	assert(!line_starts_.empty());

	if (loader_state == Loader::State::kInitial) {
		return;
	}

	// TODO memory re-alloc in Finder could cause this to block for a long time.
	//  Add a timeout parameter to finder_.user()
	auto user = finder_.user();
	for (const auto &[ctx_, job] : user.jobs()) {
		auto ctx = static_cast<FindContext *>(ctx_);
		const auto &results = job->results();

		stripe_view_.feed(ctx_, line_starts_, results);
		//
		// const auto color = ctx->view_.color();
		//
		// auto num_new = results.size() - ctx->last_report_;
		//
		// if (ctx->last_report_ == 0 || results.size() < ctx->last_report_) {
		// 	ctx->last_line_ = 0;
		// 	stripe_view_.reset();
		// }
		//
		// // const auto line_interval = line_s
		// // auto next_line = ctx->last_line_ +
		//
		// // std::cout << num_new << std::endl;
		//
		// for (size_t i = ctx->last_report_; i < results.size(); i++) {
		// 	const auto &result = results[i];
		// 	assert(result.end < line_starts_[line_starts_.size() - 1]);
		// 	assert(ctx->last_line_ < line_starts_.size());
		//
		// 	auto it = std::lower_bound(line_starts_.begin() + ctx->last_line_, line_starts_.end(), result.start);
		// 	// while (result.start >= line_starts_[ctx->last_line_]) {
		// 		// ctx->last_line_++;
		// 		// if (ctx->last_line_ >= line_starts_.size()) {
		// 			// break;
		// 		// }
		// 	// }
		// 	ctx->last_line_ = it - line_starts_.begin();
		//
		// 	float y = (float)(ctx->last_line_ - 1) / (float)(num_lines() - 1);
		// 	stripe_view_.add_point(y, color);
		// }
		// // stripe_view_.soil();
		// ctx->last_report_ = results.size();
	}
	// fflush(stdout);
	// Window::send_event();

	duration += duration_cast<microseconds>(steady_clock::now() - now);
	std::cout << "FileView::update total duration: " << duration.count() << "us\n";
}

void FileView::draw() {
	auto prev_linenum_chars = linenum_chars_;


	if (autoscroll_) {
		// jump to the end of the file
		scroll_.y = std::max(scroll_.y, max_visible_scroll().y);
	}

	linenum_chars_ = linenum_len(num_lines());

	{
		auto user = dataset_.user();
		update_buffers(content_view_.render_range_, linenum_view_.render_range_, user);
	}

	if (linenum_chars_ != prev_linenum_chars) {
		on_resize();
	}
	// TODO this is only needed if longest_line_ or num_lines_ changed
	content_view_.update_scrollbar();

	// std::cout << content_render_range.x << " " << content_render_range.y << "\n";

	// TextShader::globals.frame_offset_px = pos();

	// Timeit draw("Draw");
	//
	content_view_.draw();
	linenum_view_.draw();
	stripe_view_.draw();

	for (auto &ctx : find_ctxs_) {
		ctx->view_.draw();
	}

	// draw.stop();
}
