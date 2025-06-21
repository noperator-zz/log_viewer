#pragma once

#include "text_shader.h"
#include "widget.h"

class FileView;

class LinenumView : public Widget {
	friend class FileView;
	FileView &parent_;
	TextShader::Buffer buf_ {};
	glm::uvec2 render_range_ {};
public:
	LinenumView(FileView &parent);
	void draw() override;
};
