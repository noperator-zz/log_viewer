#pragma once

#include "text_shader.h"
#include "widget.h"

class FileView;

class LinenumView : public Widget {
	friend class FileView;
	TextShader::Buffer buf_ {};
	glm::uvec2 render_range_ {};

	void update() override;

public:
	LinenumView(Widget *parent);
	void draw() override;
};
