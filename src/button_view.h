#pragma once

#include <functional>

#include "widget.h"

class ButtonView : public Widget {
	std::function<void(bool)> on_click_;
	bool state_ {};

	bool on_mouse_button(glm::ivec2 mouse, int button, int action, Window::KeyMods mods) override;
	void update() override;

public:
	ButtonView(Widget *parent, const std::function<void(bool)> &&on_click = nullptr);

	void draw() override;
};
