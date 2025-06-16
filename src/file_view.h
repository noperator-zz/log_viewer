#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "dynarray.h"
#include "file.h"
#include "gp_shader.h"
#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"
#include "worker.h"
#include "loader.h"

class FileView;


class LinenumView : public Widget {
	friend class FileView;
	FileView &parent_;
	TextShader::Buffer buf_ {};
	glm::uvec2 render_range_ {};
public:
	LinenumView(FileView &parent);
	void draw() override;
};

class ContentView : public Widget {
	friend class FileView;

	static constexpr int SCROLL_W = 20;

	FileView &parent_;
	Scrollbar scroll_h_ {true, [this](double p){scroll_h_cb(p);}};
	Scrollbar scroll_v_ {false, [this](double p){scroll_v_cb(p);}};
	TextShader::Buffer buf_ {};
	glm::uvec2 render_range_ {};

	dynarray<TextShader::CharStyle> base_styles_ {};
	dynarray<TextShader::CharStyle> mod_styles_ {};

	std::pair<glm::ivec2, glm::ivec2> selection_abs_char_loc {};
	bool selection_active_ {false};

	void on_resize() override;
	bool on_mouse_button(glm::ivec2 mouse, int button, int action, int mods) override;
	bool on_cursor_pos(glm::ivec2 mouse) override;

	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	void update_scrollbar();

	glm::ivec2 view_pos_to_abs_char_loc(glm::ivec2 view_pos) const;
	void reset_mod_styles();
	void highlight_selection();
public:
	ContentView(FileView &parent);
	void draw() override;
};

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

	// File file_;
	Loader loader_;
	LinenumView linenum_view_ {*this};
	ContentView content_view_ {*this};

	size_t linenum_chars_ {1};
	glm::ivec2 buf_lines_ {};

	glm::ivec2 scroll_ {};

	Event quit_ {};
	std::thread loader_thread_ {};
	std::vector<size_t> line_starts_ {};
	size_t longest_line_ {};

	FileView(const char *path);

	void on_resize() override;

	void really_update_buffers(int start, int end, const uint8_t *data);
	void update_buffers(glm::uvec2 &content_render_range, glm::uvec2 &linenum_render_range, const uint8_t *data);
	void draw_lines(glm::uvec2 render_range) const;
	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	// size_t get_line_start(size_t line_idx) const;
	size_t abs_char_loc_to_abs_char_idx(const glm::ivec2 &abs_loc) const;
	size_t abs_char_idx_to_buf_char_idx(const size_t abs_idx) const;
	size_t abs_char_loc_to_buf_char_idx(const glm::ivec2 &abs_loc) const;
	size_t get_line_len(size_t line_idx) const;
	size_t num_lines() const;
public:
	static std::unique_ptr<FileView> create(const char *path);
	~FileView();

	int open();
	void scroll(glm::ivec2 scroll);

	void draw() override;
};
