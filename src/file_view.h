#pragma once

#include <chrono>
#include <condition_variable>
#include <GL/glew.h>
#include <glm/glm.hpp>

#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"
#include "loader.h"
#include "linenum_view.h"
#include "content_view.h"

class FileView : public Widget {
	friend class LinenumView;
	friend class ContentView;

	Loader loader_;
	Dataset dataset_ {nullptr, nullptr};
	Finder finder_ {dataset_};
	LinenumView linenum_view_ {this};
	ContentView content_view_ {this};
	std::vector<std::unique_ptr<FindView>> find_views_ {};

	glm::ivec2 scroll_ {};
	glm::dvec2 frac_scroll_ {};

	dynarray<size_t> line_starts_ {};
	size_t longest_line_ {};

	bool autoscroll_ {true};

	FileView(Widget *parent, const char *path);

	void on_new_lines();

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	void on_resize() override;

	void really_update_buffers(const uint8_t *data);
	void update_buffers(const Dataset::User &user);
	void scroll_to(glm::ivec2 pos, bool allow_autoscrol);
	// size_t get_line_start(size_t line_idx) const;
	static glm::ivec2 abs_char_loc_to_abs_px_loc(glm::ivec2 abs_loc) ;
	size_t abs_char_loc_to_abs_char_idx(glm::ivec2 abs_loc) const;
	size_t abs_char_idx_to_buf_char_idx(size_t abs_idx) const;
	size_t abs_char_loc_to_buf_char_idx(glm::ivec2 abs_loc) const;
	size_t get_line_len(size_t line_idx) const;
	size_t num_lines() const;
	glm::ivec2 max_scroll() const;
	glm::ivec2 max_visible_scroll() const;

	void update() override;

public:
	static std::unique_ptr<FileView> create(Widget *parent, const char *path);
	~FileView();

	int open();
	void scroll(glm::dvec2 scroll);

	void draw() override;
};
