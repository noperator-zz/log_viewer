#pragma once

#include "dynarray.h"
#include "settings.h"
#include "scrollbar.h"
#include "text_shader.h"
#include "widget.h"
#include "find_view.h"
#include "finder.h"
#include "stripe_view.h"

class FileView;

class ContentView : public Widget {
	friend class FileView;

	static constexpr int SCROLL_W = 20;

	Scrollbar scroll_h_ {this, false, [this](double p){scroll_h_cb(p);}};
	Scrollbar scroll_v_ {this, true, [this](double p){scroll_v_cb(p);}};
	StripeView stripe_view_ {this, 1000, 1};
	layout::H hlay_ {};
	layout::V vlay_ {};

	TextShader::Buffer buf_ {};
	size_t buf_num_chars_ {};
	rect<int> buf_char_window_ {};

	dynarray<TextShader::CharStyle> base_styles_ {CONTENT_BUFFER_SIZE};
	dynarray<TextShader::CharStyle> mod_styles_ {CONTENT_BUFFER_SIZE};

	glm::ivec2 cursor_abs_char_loc_ {};
	std::pair<glm::ivec2, glm::ivec2> selection_abs_char_loc {};
	bool selection_active_ {false};

	FileView &parent();

	void scroll_to_cursor(bool center);

	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	void update_scrollbar();

	glm::ivec2 view_px_loc_to_abs_char_loc(glm::ivec2 view_px_loc);
	glm::ivec2 abs_px_loc_to_view_px_loc(glm::ivec2 px_loc);

	void reset_mod_styles();
	void highlight_selection();
	void highlight_findings(Finder::User &user);

	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	bool on_cursor_pos(glm::ivec2 mouse) override;
	void on_resize() override;

	void update_from_parent(Finder::User &user);
	void update() override;

public:
	ContentView(Widget *parent);
	~ContentView();

	void draw() override;
};
