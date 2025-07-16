#pragma once

#include "dynarray.h"
#include "settings.h"
#include "text_shader.h"
#include "widget.h"

class FileView;

class LinenumView : public Widget {
	friend class FileView;
	TextShader::Buffer buf_ {};
	size_t buf_num_chars_ {};
	dynarray<TextShader::CharStyle> styles_ {LINENUM_BUFFER_SIZE};

	size_t linenum_chars_ {1};

	void update() override;

public:
	LinenumView(Widget *parent);
	void draw() override;
};
