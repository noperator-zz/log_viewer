#pragma once

#include "dynarray.h"
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
	std::vector<std::unique_ptr<FindView>> find_views_ {};
	TextShader::Buffer buf_ {};
	glm::uvec2 render_range_ {};

	dynarray<TextShader::CharStyle> base_styles_ {};
	dynarray<TextShader::CharStyle> mod_styles_ {};

	std::pair<glm::ivec2, glm::ivec2> selection_abs_char_loc {};
	bool selection_active_ {false};

	FileView &parent();

	void on_find(void *ctx, size_t idx);
	void on_findview_event(FindView &view, FindView::Event event);

	void scroll_h_cb(double percent);
	void scroll_v_cb(double percent);
	void update_scrollbar();

	glm::ivec2 view_pos_to_abs_char_loc(glm::ivec2 view_pos);
	void reset_mod_styles();
	void highlight_selection();
	void highlight_findings(Finder::User &user);

	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	bool on_cursor_pos(glm::ivec2 mouse) override;
	void on_resize() override;

	void update() override;

public:
	ContentView(Widget *parent);
	~ContentView();

	void draw() override;
};
