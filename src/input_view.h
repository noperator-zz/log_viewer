#pragma once

#include <functional>

#include "text_shader.h"
#include "widget.h"

class InputView : public Widget {
	static constexpr size_t BUFFER_SIZE = 1024 * 1024;

	std::function<void(std::string_view)> on_update_;
	std::string text_ {};
	size_t cursor_ {};
	TextShader::Buffer buf_ {};

	bool on_key(int key, int scancode, int action, Window::KeyMods mods) override;
	bool on_char(unsigned int codepoint, Window::KeyMods mods) override;
	void on_resize() override;

public:
	InputView(const std::function<void(std::string_view)> &&on_update = nullptr);

	std::string_view text() const;
	void draw() override;
};
