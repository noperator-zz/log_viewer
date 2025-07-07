#pragma once

#include <functional>

#include "widget.h"

class ButtonView : public Widget {
	const bool toggle_;
	std::function<void(bool)> on_click_;
	bool enabled_ {};
	bool state_ {};

	void on_hover(bool state) override;
	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	void update() override;

public:
	ButtonView(Widget *parent, bool toggle = false, const std::function<void(bool)> &&on_click = nullptr);

	void set_enabled(bool enabled);
	void draw() override;
};
