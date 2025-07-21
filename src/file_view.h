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

	struct FindContext {
		FindView view;
		// Indices of the lines that contain one or more matches
		dynarray<size_t> line_indices {};
		size_t next_match_idx_ {};
		size_t next_line_idx_ {};

		// TODO this is basically a copy of the FindView constructor, but the generic alternative is even uglier.
		FindContext(Widget *parent, color color, std::function<void(FindView &, FindView::Event)> &&event_cb) :
			view(parent, color, std::move(event_cb)) {
		}

		void reset() {
			line_indices.resize_uninitialized(0);
			next_match_idx_ = 0;
			next_line_idx_ = 0;
		}

		void feed(const dynarray<size_t> &line_starts, const dynarray<Finder::Job::Result> &results) {
			const size_t num_lines = line_starts.size();

			// O(N + M) linear scan.
			for (; next_match_idx_ < results.size(); next_match_idx_++) {
				const auto &result = results[next_match_idx_];
				if (result.start < line_starts[next_line_idx_]) {
					continue; // skip multiple matches on the same line
				}
				while (1) {
					assert(next_line_idx_ < num_lines);
					if (result.start < line_starts[next_line_idx_]) {
						// TODO this can block during realloc
						line_indices.push_back(next_line_idx_ - 1);
						break;
					}
					next_line_idx_++;
				}
			}
		}
	};

	InputProcessor loader_;
	Dataset dataset_ {nullptr, nullptr};
	Finder finder_ {dataset_};
	LinenumView linenum_view_ {this};
	ContentView content_view_ {this};
	std::unordered_map<FindView *, std::unique_ptr<FindContext>> find_ctxs_ {};

	glm::ivec2 scroll_ {};
	glm::dvec2 frac_scroll_ {};

	dynarray<size_t> line_starts_ {};
	size_t longest_line_ {};
	// Array of indices into line_starts_, computed based on which lines are visible after filtering
	dynarray<size_t> filtered_line_indices_ {};

	bool autoscroll_ {true};

	FileView(Widget *parent, const char *path);

	void on_new_lines();
	void on_findview_event(FindView &view, FindView::Event event);
	void on_finder_results(void *ctx, size_t idx);

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	void on_resize() override;

	void really_update_buffers(const uint8_t *data);
	bool update_buffers(const Dataset::User &user);
	void scroll_to(glm::ivec2 pos, bool allow_autoscroll);
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
