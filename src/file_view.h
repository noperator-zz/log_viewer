#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>
#include <utility>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "file.h"
#include "gp_shader.h"
#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"
#include "worker.h"
#include "loader.h"

class FileView;


class LinenumView : public Widget {
	FileView &parent_;
public:
	LinenumView(FileView &parent);
	void draw() override;
};

class ContentView : public Widget {
	FileView &parent_;
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

	static constexpr int SCROLL_W = 20;

	// File file_;
	Loader loader_;
	LinenumView linenum_view_ {*this};
	ContentView content_view_ {*this};
	Scrollbar scroll_h_ {true, [this](double p){scroll_h_cb(p);}};
	Scrollbar scroll_v_ {false, [this](double p){scroll_v_cb(p);}};
	TextShader::Buffer content_buf_ {};
	TextShader::Buffer linenum_buf_ {};
	glm::uvec2 content_render_range_ {};
	glm::uvec2 linenum_render_range_ {};

	// struct RenderParams {
	// 	std::vector<size_t> line_starts_ {};
	// 	size_t linenum_chars_ {1};
	// 	// First and last lines in the buffer
	// 	glm::ivec2 buf_lines_ {};
	// 	size_t num_lines_ {0};
	// 	size_t longest_line_ {0};
	// buf_offset
	// };
	// RenderParams params_ {};

	// std::vector<size_t> line_starts_ {};
	size_t linenum_chars_ {1};
	// First and last lines in the buffer
	glm::ivec2 buf_lines_ {};

	glm::ivec2 scroll_ {};

	Event quit_ {};
	std::thread loader_thread_ {};
	Loader::State state_ {};
	size_t num_lines_ {};
	size_t longest_line_ {};

	FileView(const char *path);

	void on_resize() override;
	void update_scrollbar();

	void really_update_buffers(int start, int end, const Loader::Snapshot &snapshot);
	void update_buffers(glm::uvec2 &content_render_range, glm::uvec2 &linenum_render_range, const Loader::Snapshot &snapshot);
	void draw_lines(glm::uvec2 render_range) const;
	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	static size_t get_line_start(Loader::State state, size_t line_idx, const std::vector<size_t> &line_ends);
	static size_t get_line_len(Loader::State state, size_t line_idx, const std::vector<size_t> &line_ends);
public:
	static std::unique_ptr<FileView> create(const char *path);
	~FileView();

	int open();
	void scroll(glm::ivec2 scroll);

	void draw() override;
};
