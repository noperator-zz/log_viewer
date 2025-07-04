#pragma once

#include <chrono>
#include <condition_variable>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"
#include "loader.h"
#include "linenum_view.h"
#include "content_view.h"
#include "find_view.h"
#include "finder.h"
#include "stripe_view.h"

class FileView : public Widget {
	friend class LinenumView;
	friend class ContentView;
	// TODO make these limits dynamic
	static constexpr size_t MAX_SCREEN_LINES = 200;
	static constexpr ssize_t OVERSCAN_LINES = 1;
	static constexpr size_t MAX_LINE_LENGTH = 16 * 1024;
	static constexpr size_t CONTENT_BUFFER_SIZE = (MAX_SCREEN_LINES + OVERSCAN_LINES * 2) * MAX_LINE_LENGTH * (sizeof(TextShader::CharStyle) + sizeof(uint8_t));
	static constexpr size_t LINENUM_BUFFER_SIZE = (MAX_SCREEN_LINES + OVERSCAN_LINES * 2) * 10 * (sizeof(TextShader::CharStyle) + sizeof(uint8_t));

	static_assert(CONTENT_BUFFER_SIZE < 128 * 1024 * 1024, "Content buffer size too large");
	static_assert(OVERSCAN_LINES >= 1, "Overscan lines must be at least 1");

	struct FindContext {
		FindView view_;

		// State as of the last update
		// # lines used for position calculation.
		// Latched until the actual # lines increases by (1 / resolution) %, at which point
		// the value is updated, and the stripe view is reset and recomputed.
		size_t num_lines_at_reset_ {};

		size_t last_report_ {};
		size_t last_line_ {};

		FindContext(auto&& cb)
		: view_{std::forward<decltype(cb)>(cb)} {}
	};

	Loader loader_;
	Dataset dataset_ {nullptr, nullptr};
	Finder finder_ {dataset_};
	LinenumView linenum_view_ {*this};
	ContentView content_view_ {*this};
	std::vector<std::unique_ptr<FindContext>> find_ctxs_ {};
	StripeView stripe_view_ {1000, 1};

	size_t linenum_chars_ {1};
	glm::ivec2 buf_lines_ {};

	glm::ivec2 scroll_ {};

	dynarray<size_t> line_starts_ {};
	size_t longest_line_ {};

	bool autoscroll_ {true};

	FileView(const char *path);

	void on_new_lines();
	void on_find(void *ctx, size_t idx);

	void handle_findview(FindView &find_view);
	void on_resize() override;

	void really_update_buffers(int start, int end, const uint8_t *data);
	void update_buffers(glm::uvec2 &content_render_range, glm::uvec2 &linenum_render_range, const Dataset::User &user);
	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	// size_t get_line_start(size_t line_idx) const;
	size_t abs_char_loc_to_abs_char_idx(glm::ivec2 abs_loc) const;
	size_t abs_char_idx_to_buf_char_idx(size_t abs_idx) const;
	size_t abs_char_loc_to_buf_char_idx(glm::ivec2 abs_loc) const;
	size_t get_line_len(size_t line_idx) const;
	size_t num_lines() const;
	glm::ivec2 max_scroll() const;
	glm::ivec2 max_visible_scroll() const;

	void update() override;

public:
	static std::unique_ptr<FileView> create(const char *path);
	~FileView();

	int open();
	void scroll(glm::ivec2 scroll);

	void draw() override;
};
