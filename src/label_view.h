#pragma once

#include "text_shader.h"
#include "widget.h"

class LabelView : public Widget {
	static constexpr size_t BUFFER_SIZE = 1024 * 1024;

	std::string text_ {};
	TextShader::Buffer buf_ {};

	void on_resize() override;
	void update() override;

public:
	LabelView(Widget *parent);

	void set_text(const std::string &text);
	void draw() override;
};
