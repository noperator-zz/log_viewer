#pragma once

#include <functional>

#include "widget.h"
#include "res/tex.h"

class ButtonView : public Widget {
	struct TexSet {
		TexID idle;
		TexID hover;
		TexID active;
		TexID disabled;

		TexSet(TexID single_tex = TexID::None)
			: idle(single_tex), hover(single_tex), active(single_tex), disabled(single_tex) {}
		TexSet(TexID idle_tex, TexID hover_tex, TexID active_tex, TexID disabled_tex)
			: idle(idle_tex), hover(hover_tex), active(active_tex), disabled(disabled_tex) {}
	};

	TexSet tex_set_ {};
	std::function<void()> on_click_;
	bool enabled_ {};
	bool state_ {};

	void on_hover(bool state) override;
	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	void update() override;

public:
	ButtonView(Widget *parent, TexSet tex_set = {}, const std::function<void()> &&on_click = nullptr);

	// bool state() const { return state_; }
	void set_state(bool state);
	void set_enabled(bool enabled);
	void draw() override;
};
