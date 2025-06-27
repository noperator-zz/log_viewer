#pragma once

#include "dynarray.h"
#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"

class FileView;

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
	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	bool on_cursor_pos(glm::ivec2 mouse) override;

	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	void update_scrollbar();

	glm::ivec2 view_pos_to_abs_char_loc(glm::ivec2 view_pos) const;
	void reset_mod_styles();
	void highlight_selection();
	void highlight_findings();
public:
	ContentView(FileView &parent);
	void draw() override;
};
